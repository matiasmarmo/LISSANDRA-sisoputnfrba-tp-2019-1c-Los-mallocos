#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>
#include <time.h>
#include <errno.h>
#include <sys/file.h>
#include <commons/config.h>
#include <commons/string.h>

#include "../lfs-config.h"
#include "filesystem.h"
#include "manejo-tablas.h"
#include "config-with-locks.h"
#include "manejo-datos.h"
#include "bitmap.h"
#include "config-with-locks.h"
#include "../lissandra/memtable.h"
#include "../lfs-logger.h"

#define TIMESTAMP_STR_LEN 20

int _local_get_tamanio_value() {
	// Wrapper para llamar a get_tamanio_value solo una vez
	// y no constantemente cada vez que se itera, así
	// evitamos el overhead de bloquear el semáforo
	// de lfs-config
	static int tamanio = -1;
	if (tamanio == -1) {
		tamanio = get_tamanio_value();
	}
	return tamanio;
}

int get_tamanio_string_registro() {
	// El tamanio maximo del string de un registro es:
	// El tamanio del value +
	// 20 bytes del timestamp +
	// 5 bytes de la key +
	// 2 bytes de los ';' +
	// 1 byte del '\0'

	// Por la misma razón que en _local_get_tamanio_value(),
	// almacenamos el valor
	static int tamanio = -1;
	if (tamanio == -1) {
		tamanio = _local_get_tamanio_value() + TIMESTAMP_STR_LEN + 5 + 2 + 1;
	}
	return tamanio;
}

int path_a_bloque(char *numero_bloque, char *buffer, int tamanio_buffer) { //obtiene el path del bloque
	if (tamanio_buffer < TAMANIO_PATH) {
		return -1;
	}
	sprintf(buffer, "%sBloques/%s.bin", get_punto_montaje(), numero_bloque);
	return 0;
}

void obtener_path_bloque(int nro_bloque, char* path_bloque) {
	char nro_bloque_string[20];
	sprintf(nro_bloque_string, "%d", nro_bloque);
	path_a_bloque(nro_bloque_string, path_bloque, TAMANIO_PATH);
}

FILE *_abrir_archivo_con_lock(char *path, char *modo, int tipo_lock) {

	// Abre el archivo con dirección 'path', en el modo de lectura
	// 'modo' ("r", "w", etc) y aplica un lock del tipo 'tipo_lock'.
	// El tipo de lock puede ser:
	//      LOCK_SH: lock compartido (el que usamos para lectura)
	//      LOCK_EX: lock exclusivo (el que usamos para escritura)

	FILE *archivo;
	int file_descriptor;
	if ((archivo = fopen(path, modo)) == NULL) {
		return NULL;
	}
	file_descriptor = fileno(archivo);
	if (file_descriptor == -1 || flock(file_descriptor, tipo_lock) < 0) {
		fclose(archivo);
		return NULL;
	}
	return archivo;
}

FILE *abrir_archivo_para_lectura(char *path) {

	// Abre el archivo 'path' para lectura con un lock compartido

	return _abrir_archivo_con_lock(path, "r+", LOCK_SH);
}

FILE *abrir_archivo_para_escritura(char *path) {

	// Abre el archivo 'path' para escritura con un lock exclusivo

	return _abrir_archivo_con_lock(path, "w", LOCK_EX);
}

FILE *abrir_archivo_para_lectoescritura(char *path) {
	FILE *archivo = _abrir_archivo_con_lock(path, "r+", LOCK_EX);
	if (archivo == NULL && errno == ENOENT) {
		// No existe, lo creamos
		archivo = _abrir_archivo_con_lock(path, "w+", LOCK_EX);
	}
	return archivo;
}

FILE *abrir_archivo_para_agregar(char *path) {

	// Abre el archivo 'path' para agregar datos al final con un lock exclusivo

	return _abrir_archivo_con_lock(path, "a", LOCK_EX);
}

int parsear_registro(char *string_registro, registro_t *registro) {

	// Recibe el string de un registro con el formato "TIMESTAMP;KEY;VALUE"
	// y un puntero a un registro_t.
	// Parsea el string y carga en el registro los valores.

	char buffer[get_tamanio_string_registro()];
	int indice_string_registro = 0, indice_buffer = 0;

	void _siguiente_substring() {
		memset(buffer, 0, get_tamanio_string_registro());
		while (string_registro[indice_string_registro] != ';'
				&& string_registro[indice_string_registro] != '\0') {
			buffer[indice_buffer++] = string_registro[indice_string_registro++];
		}
		indice_string_registro++;
		indice_buffer = 0;
	}

	_siguiente_substring();
	registro->timestamp = (time_t) strtoumax(buffer, NULL, 10);
	_siguiente_substring();
	registro->key = (uint16_t) strtoumax(buffer, NULL, 10);
	_siguiente_substring();
	registro->value = malloc(strlen(buffer) + 1);
	if (registro->value == NULL) {
		return -1;
	}
	strcpy(registro->value, buffer);
	return 0;
}

int leer_hasta_siguiente_token(FILE *archivo, char *resultado, char token) {

	// Recibe un archivo, un string 'resultado' y un caracter 'token'.
	// Le el archivo hasta encontrar el token y graba lo que lee en
	// 'resultado' sin incluir el token.
	// Retorna:
	//      * -1 en caso de error
	//      * 0 si se llegó al EOF
	//      * 1 si salió todo bien

	char caracter_leido, buffer_cat[2] = { 0 };
	if (fread(&caracter_leido, 1, 1, archivo) != 1) {
		// Retornamos 0 si termino el archivo, -1 ante un error
		return feof(archivo) ? 0 : -1;
	}
	while (caracter_leido != token) {
		buffer_cat[0] = caracter_leido;
		strcat(resultado, buffer_cat);
		if (fread(&caracter_leido, 1, 1, archivo) != 1) {
			// Retornamos 0 si termino el archivo, -1 ante un error
			return feof(archivo) ? 0 : -1;
		}
	}
	return 1;
}

int iterar_bloque(char *path_bloque, char *registro_truncado,
		operacion_t operacion) {

	// Recibe la dirección de un bloque en 'path_bloque' y por cada
	// registro del bloque ejecuta 'operación' con el registro_t
	// correspondiente como parámetro. En caso de que la operación
	// retorne algo distínto de CONTINUAR, se cortará la iteración.

	// Por medio de 'registro_truncado' recibe el posible registro
	// que haya quedado incompleto por una iteración a un bloque
	// anterior para así completarlo. A su vez, si hay un registro
	// cortado al final de este bloque, se almacena en 'registro_truncado'

	FILE *archivo_bloque = abrir_archivo_para_lectura(path_bloque);
	char string_registro[get_tamanio_string_registro()];
	int resultado;
	registro_t registro;
	// Si había un registro truncado, lo restauramos
	memset(string_registro, 0, get_tamanio_string_registro());
	strcpy(string_registro, registro_truncado);
	memset(registro_truncado, 0, get_tamanio_string_registro());
	if (archivo_bloque == NULL) {
		return errno == ENOENT ? FINALIZAR : -1;
	}

	while ((resultado = leer_hasta_siguiente_token(archivo_bloque,
			string_registro, '\n')) == 1) {
		if (parsear_registro(string_registro, &registro) < 0) {
			resultado = -1;
			break;
		}
		if ((resultado = operacion(registro)) != CONTINUAR) {
			break;
		}
		memset(string_registro, 0, get_tamanio_string_registro());
	}

	fclose(archivo_bloque);

	if (resultado == 0) {
		// EOF, manejamos el posible registro truncado
		strcpy(registro_truncado, string_registro);
		return CONTINUAR;
	}
	return resultado;
}

int iterar_archivo_de_datos(char *path, operacion_t operacion) {

	// Itera todos los bloques asociados al archivo de datos
	// (partición o temporal) 'path' usando la función
	// 'iterar_bloque' en cada uno y ejecutando 'operacion'
	// por cada registro. También maneja los registros truncados
	// entre bloques.

	FILE *archivo = abrir_archivo_para_lectura(path);
	if (archivo == NULL) {
		return -1;
	}
	t_config *archivo_datos = lfs_config_create_from_file(path, archivo);
	if (archivo_datos == NULL) {
		fclose(archivo);
		return -1;
	}

	char registro_truncado[get_tamanio_string_registro()];
	char path_bloque[TAMANIO_PATH] = { 0 };
	int resultado;

	memset(registro_truncado, 0, get_tamanio_string_registro());

	char **bloques = config_get_array_value(archivo_datos, "BLOCKS");
	if (bloques == NULL) {
		config_destroy(archivo_datos);
		fclose(archivo);
		return -1;
	}

	for (char **bloque_num = bloques; *bloque_num != NULL; bloque_num++) {
		path_a_bloque(*bloque_num, path_bloque, TAMANIO_PATH);
		resultado = iterar_bloque(path_bloque, registro_truncado, operacion);
		if (resultado != CONTINUAR) {
			break;
		}
	}

	fclose(archivo);

	for (char **bloque_num = bloques; *bloque_num != NULL; bloque_num++) {
		free(*bloque_num);
	}
	free(bloques);
	config_destroy(archivo_datos);
	return resultado < 0 ? -1 : 0;
}

int iterar_particion(char* nombre_tabla, int numero_particion,
		operacion_t operacion) {
	char path_particion[TAMANIO_PATH];
	bloquear_tabla(nombre_tabla, 'r');
	obtener_path_particion(numero_particion, nombre_tabla, path_particion);
	int res = iterar_archivo_de_datos(path_particion, operacion);
	desbloquear_tabla(nombre_tabla);
	return res;
}

int iterar_archivo_temporal(char *nombre_tabla, int numero_temporal,
		operacion_t operacion) {
	char path_particion[TAMANIO_PATH];
	bloquear_tabla(nombre_tabla, 'r');
	obtener_path_temporal(numero_temporal, nombre_tabla, path_particion);
	int res = iterar_archivo_de_datos(path_particion, operacion);
	desbloquear_tabla(nombre_tabla);
	return res;
}

int leer_archivo_de_datos(char *path, registro_t **resultado) {

	// Recibe un archivo de datos 'path' y un doble puntero
	// a registro. Aloca memoria al puntero y graba en él
	// todos los registros del archivo de datos.

	// Arrancamos con lugar para 10 registros
	int tamanio_actual = 10 * sizeof(registro_t), registros_cargados = 0;
	bool hubo_error = false;

	*resultado = malloc(tamanio_actual);
	if (*resultado == NULL) {
		return -1;
	}

	int _cargar_registro(registro_t registro) {
		registro_t *puntero_realocado;
		if ((registros_cargados * sizeof(registro_t)) >= tamanio_actual) {
			tamanio_actual *= 2;
			puntero_realocado = realloc(*resultado, tamanio_actual);
			if (puntero_realocado == NULL) {
				hubo_error = true;
				return FINALIZAR;
			}
			*resultado = puntero_realocado;
		}
		(*resultado)[registros_cargados++] = registro;
		return CONTINUAR;
	}

	if (iterar_archivo_de_datos(path, &_cargar_registro) < 0 || hubo_error) {
		// Error, liberar resultado
		for (int i = 0; i < registros_cargados; i++) {
			free((*resultado)[i].value);
		}
		free(*resultado);
		return -1;
	}

	return registros_cargados;
}

int leer_particion(char* nombre_tabla, int numero_particion,
		registro_t **registros) {
	char path_particion[TAMANIO_PATH];
	obtener_path_particion(numero_particion, nombre_tabla, path_particion);
	return leer_archivo_de_datos(path_particion, registros);
}

int borrar_bloque(int bloque) {
	char path_bloque[TAMANIO_PATH];
	obtener_path_bloque(bloque, path_bloque);
	int fd;
	FILE *archivo_bloque = abrir_archivo_para_lectoescritura(path_bloque);

	if (archivo_bloque == NULL) {
		return -1;
	}

	fd = fileno(archivo_bloque);

	if (ftruncate(fd, 0) < 0) {
		fclose(archivo_bloque);
		return -1;
	}
	fclose(archivo_bloque);

	if (liberar_bloque_bitmap(bloque) < 0) {
		return -1;
	}
	return 0;
}

int iterar_bloques(char** bloques, operacion_bloque_t operacion_bloque) {
	int hubo_error = 0;
	for (char **i = bloques; *i != NULL; i++) {
		if (operacion_bloque((int) strtoumax(*i, NULL, 10)) < 0) {
			hubo_error = -1;
		}
	}
	return hubo_error;
}

int liberar_bloques_de_archivo(t_config *config_archivo) {
	char** buffer = config_get_array_value(config_archivo, "BLOCKS");
	int hubo_error = 0;
	if (iterar_bloques(buffer, &borrar_bloque) < 0) {
		hubo_error = -1;
	}

	for (char **i = buffer; *i != NULL; i++) {
		free(*i);
	}
	free(buffer);

	return hubo_error;
}

int pisar_particion(char* tabla, int nro_particion, registro_t* registros,
		int cantidad) {

	char path_particion[TAMANIO_PATH];
	char blocks_str[10];
	obtener_path_particion(nro_particion, tabla, path_particion);
	FILE* archivo = abrir_archivo_para_lectoescritura(path_particion);
	t_config* config_particion = lfs_config_create_from_file(path_particion,
			archivo);

	// No chequeamos el error de liberar_bloques_particion
	// porque, incluso si falla, igual debemos
	// continuar con la misma ejecución
	liberar_bloques_de_archivo(config_particion);

	int bloque_temp = pedir_bloque_bitmap();
	if (bloque_temp < 0) {
		// Esto no debería pasar porque acabamos de liberar
		// nosotros bloques y siempre debiera haber alguno
		// disponible.
		// Pero, en caso de que falle, estamos dejando
		// al archivo en un estado inconsistente (tiene
		// los bloques que le liberamos)
		// Por el momento, asumimos que no va a fallar.
		fclose(archivo);
		config_destroy(config_particion);
		return -1;
	}
	sprintf(blocks_str, "[%d]", bloque_temp);
	config_set_value(config_particion, "BLOCKS", blocks_str);
	config_set_value(config_particion, "SIZE", "0");
	lfs_config_save_in_file(config_particion, archivo);

	config_destroy(config_particion);
	fclose(archivo);

	if (escribir_en_archivo_de_datos(path_particion, registros, cantidad) < 0) {
		return -1;
	}

	return 0;
}

void registro_a_string(registro_t registro, char *buffer) {
	sprintf(buffer, "%lu;%d;%s", registro.timestamp, registro.key,
			registro.value);
}

char *array_de_registros_a_string(registro_t *registros, int cantidad) {
	char buffer[get_tamanio_string_registro()];
	int tamanio_actual = 10 * get_tamanio_string_registro();
	char *resultado = malloc(tamanio_actual);
	int cantidad_cargados = 0;
	int registro_siguiente = 0;
	if (resultado == NULL) {
		return NULL;
	}
	memset(buffer, 0, get_tamanio_string_registro());
	memset(resultado, 0, tamanio_actual);
	for (int i = 0; i < cantidad; i++) {
		registro_a_string(registros[registro_siguiente++], buffer);
		if ((cantidad_cargados + 1) * get_tamanio_string_registro() >= tamanio_actual) {
			tamanio_actual *= 2;
			char *realocado = realloc(resultado, tamanio_actual);
			if (realocado == NULL) {
				free(resultado);
				return NULL;
			}
			resultado = realocado;
		}
		strcat(resultado, buffer);
		strcat(resultado, "\n");
		cantidad_cargados++;
	}

	return resultado;
}

int cantidad_bytes_a_escribir(registro_t *registros, int cantidad_registros) {
	int resultado = 0;
	char buffer[get_tamanio_string_registro()];
	for (int i = 0; i < cantidad_registros; i++) {
		registro_a_string(registros[i], buffer);
		// Agregamos un caracter extra por el '\n'
		resultado += strlen(buffer) + 1;
	}
	return resultado;
}

int bytes_disponibles_en_bloque_from_file(FILE *archivo) {
	long bytes_actuales;
	if (fseek(archivo, 0, SEEK_END) < 0) {
		fclose(archivo);
		return -1;
	}

	if ((bytes_actuales = ftell(archivo)) < 0) {
		fclose(archivo);
		return -1;
	}

	return (int) (get_tamanio_bloque() - bytes_actuales);
}

int bytes_disponibles_en_bloque(char *numero_bloque) {
	char path_bloque[TAMANIO_PATH] = { 0 };
	path_a_bloque(numero_bloque, path_bloque, TAMANIO_PATH);

	int resultado;
	FILE *archivo = abrir_archivo_para_lectura(path_bloque);
	if (archivo == NULL) {
		return errno == ENOENT ? get_tamanio_bloque() : -1;
	}

	resultado = bytes_disponibles_en_bloque_from_file(archivo);

	fclose(archivo);
	return resultado;
}

int ultimo_bloque_de_archivo_de_datos(char *path, FILE *file) {
	t_config *archivo_datos = lfs_config_create_from_file(path, file);
	char **i;
	char numero_bloque[10];
	if (archivo_datos == NULL) {
		return -1;
	}
	char **bloques = config_get_array_value(archivo_datos, "BLOCKS");
	if (bloques == NULL) {
		config_destroy(archivo_datos);
		return -1;
	}

	for (i = bloques; *i != NULL; i++)
		;
	strcpy(numero_bloque, *(--i));

	for (i = bloques; *i != NULL; i++) {
		free(*i);
	}
	free(bloques);
	config_destroy(archivo_datos);

	return (int) (strtoumax(numero_bloque, NULL, 10));
}

int bytes_disponibles_en_ultimo_bloque(int numero_bloque) {
	char numero_str[10];
	sprintf(numero_str, "%d", numero_bloque);
	return bytes_disponibles_en_bloque(numero_str);
}

int escribir_en_bloque(int numero_bloque, char *string_datos, int desde) {
	char numero_bloque_str[10], path_bloque[TAMANIO_PATH];
	sprintf(numero_bloque_str, "%d", numero_bloque);
	path_a_bloque(numero_bloque_str, path_bloque, TAMANIO_PATH);

	FILE *archivo_bloque = abrir_archivo_para_lectoescritura(path_bloque);
	if (archivo_bloque == NULL) {
		return -1;
	}

	int bytes_disponibles = bytes_disponibles_en_bloque_from_file(
			archivo_bloque);
	char *string_a_escribir = &(string_datos[desde]);
	int bytes_a_escribir;
	if (bytes_disponibles < 0 || fseek(archivo_bloque, 0, SEEK_SET) < 0) {
		fclose(archivo_bloque);
		return -1;
	}
	bytes_a_escribir =
			strlen(string_a_escribir) > bytes_disponibles ?
					bytes_disponibles : strlen(string_a_escribir);
	if (fseek(archivo_bloque, 0, SEEK_END) < 0) {
		fclose(archivo_bloque);
		return -1;
	}
	if (fwrite(&(string_datos[desde]), bytes_a_escribir, 1, archivo_bloque)
			!= 1) {
		fclose(archivo_bloque);
		return -1;
	}
	fclose(archivo_bloque);
	return bytes_a_escribir;
}

int agregar_bloque_a_config(t_config *config, int bloque) {

	char numero[10];
	sprintf(numero, "%d", bloque);
	char *bloques_actuales = config_get_string_value(config, "BLOCKS");
	char *nuevo_valor = malloc(strlen(bloques_actuales) + 10);
	if (nuevo_valor == NULL) {
		free(bloques_actuales);
		return -1;
	}
	strcpy(nuevo_valor, bloques_actuales);
	int indice = 0;
	while (nuevo_valor[indice++] != ']')
		;
	nuevo_valor[indice - 1] = ',';
	nuevo_valor[indice] = ' ';
	nuevo_valor[indice + 1] = '\0';
	strcat(nuevo_valor, numero);
	strcat(nuevo_valor, "]");
	config_set_value(config, "BLOCKS", nuevo_valor);
	free(nuevo_valor);
	return 0;
}

void sumar_bytes_escritos_a_config(t_config *config, int cantidad) {
	char valor[10] = { 0 };
	sprintf(valor, "%d", cantidad + config_get_int_value(config, "SIZE"));
	config_set_value(config, "SIZE", valor);
}

int escribir_datos_en_bloques(char *path, FILE *archivo_datos, int *bloques,
		int cant_bloques, char *string_a_escribir) {
	int caracteres_escritos = 0, ret_escritura;
	t_config *config_datos = lfs_config_create_from_file(path, archivo_datos);
	if (config_datos == NULL) {
		return -1;
	}

	ret_escritura = escribir_en_bloque(bloques[0], string_a_escribir,
			caracteres_escritos);
	if (ret_escritura == -1) {
		config_destroy(config_datos);
		return -1;
	}
	caracteres_escritos += ret_escritura;
	for (int i = 1; i < cant_bloques; i++) {
		ret_escritura = escribir_en_bloque(bloques[i], string_a_escribir,
				caracteres_escritos);
		if (ret_escritura == -1) {
			config_destroy(config_datos);
			return -1;
		}
		caracteres_escritos += ret_escritura;
		if (agregar_bloque_a_config(config_datos, bloques[i]) < 0) {
			config_destroy(config_datos);
			return -1;
		}
	}
	sumar_bytes_escritos_a_config(config_datos, caracteres_escritos);
	lfs_config_save_in_file(config_datos, archivo_datos);
	config_destroy(config_datos);
	return 0;
}

int liberar_bloques_bitmap(int *bloques, int cantidad) {
	int hubo_error = 0;
	for (int i = 0; i < cantidad; i++) {
		if (liberar_bloque_bitmap(bloques[i]) < 0) {
			hubo_error = -1;
		}
	}
	return hubo_error;
}

int cantidad_de_bloques_a_pedir(int cantiadad_bytes_a_escribir,
		int restante_ultimo_bloque) {
	int res;
	if (cantiadad_bytes_a_escribir < restante_ultimo_bloque) {
		res = 0;
	} else {
		res = (cantiadad_bytes_a_escribir - restante_ultimo_bloque)
				/ get_tamanio_bloque() + 1;
		if ((cantiadad_bytes_a_escribir - restante_ultimo_bloque)
				% get_tamanio_bloque() == 0) {
			res--;
		}
	}
	return res;
}

int pedir_bloques(int *bloques, int cantidad, int ultimo_bloque) {
	bloques[0] = ultimo_bloque;
	for (int i = 1; i < cantidad; i++) {
		int bloque = pedir_bloque_bitmap();
		if (bloque == -1) {
			liberar_bloques_bitmap(bloques + 1, i - 1);
			return -1;
		}
		bloques[i] = bloque;
	}
	return 0;
}

int escribir_en_archivo_de_datos(char *path, registro_t *registros,
		int cantidad_registros) {
	FILE *archivo = abrir_archivo_para_lectoescritura(path);
	if (archivo == NULL) {
		return -1;
	}
	int cantidad_de_bytes = cantidad_bytes_a_escribir(registros,
			cantidad_registros);
	int ultimo_bloque = ultimo_bloque_de_archivo_de_datos(path, archivo);
	int restante_ultimo_bloque = bytes_disponibles_en_ultimo_bloque(
			ultimo_bloque);
	int cantidad_bloques_que_pedir = cantidad_de_bloques_a_pedir(
			cantidad_de_bytes, restante_ultimo_bloque);

	int bloques_a_escribir[cantidad_bloques_que_pedir + 1];
	if (pedir_bloques(bloques_a_escribir, cantidad_bloques_que_pedir + 1,
			ultimo_bloque) < 0) {
		fclose(archivo);
		return -1;
	}

	char *string_a_escribir = array_de_registros_a_string(registros,
			cantidad_registros);
	if (string_a_escribir == NULL) {
		liberar_bloques_bitmap(bloques_a_escribir + 1,
				cantidad_bloques_que_pedir);
		fclose(archivo);
		return -1;
	}
	if (escribir_datos_en_bloques(path, archivo, bloques_a_escribir,
			cantidad_bloques_que_pedir + 1, string_a_escribir) < 0) {
		liberar_bloques_bitmap(bloques_a_escribir + 1,
				cantidad_bloques_que_pedir);
		free(string_a_escribir);
		fclose(archivo);
		return -1;
	}
	free(string_a_escribir);
	fclose(archivo);
	return 0;
}

int bajar_a_archivo_temporal(char* tabla, registro_t* registros,
		int cantidad_registros) {
	char path_tmp[TAMANIO_PATH] = { 0 };
	int existe_tabla_res;
	int numero;

	existe_tabla_res = existe_tabla(tabla);
	if (existe_tabla_res == -1) {
		return -1;
	} else if (existe_tabla_res == 1) {
		return TABLA_NO_EXISTENTE;
	}

	if ((numero = cantidad_tmp_en_tabla(tabla)) < 0) {
		return -1;
	}

	bloquear_tabla(tabla, 'r');

	if (crear_temporal(numero, tabla) < 0) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al crear temporal de la tabla %s", tabla);
		desbloquear_tabla(tabla);
		return -1;
	}

	obtener_path_temporal(numero, tabla, path_tmp);

	if ((escribir_en_archivo_de_datos(path_tmp, registros, cantidad_registros))
			== -1) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al escribir en archivo de datos de la tabla %s", tabla);
		desbloquear_tabla(tabla);
		return -1;
	}

	desbloquear_tabla(tabla);
	return 0;
}

int obtener_datos_de_tmpcs(char *tabla, registro_t **resultado) {
	int tamanio_actual = 10 * sizeof(registro_t);
	int cantidad_actual_registros = 0;
	int hubo_error = 0;
	*resultado = malloc(tamanio_actual);
	if (*resultado == NULL) {
		return -1;
	}

	int _concatenar_registros_al_resultado(registro_t *registros, int cantidad) {
		while ((cantidad_actual_registros + cantidad) * sizeof(registro_t) >= tamanio_actual) {
			tamanio_actual *= 2;
			registro_t *realocado = realloc(*resultado, tamanio_actual);
			if (realocado == NULL) {
				hubo_error = -1;
				return -1;
			}
			*resultado = realocado;
		}
		memcpy((*resultado) + cantidad_actual_registros, registros,
				cantidad * sizeof(registro_t));
		cantidad_actual_registros += cantidad;
		return 0;
	}

	int _leer_archivo(const char* path, const struct stat* stat, int flag) {
		if (string_ends_with((char*) path, ".tmpc")) {
			registro_t *nuevos_registros = NULL;
			int cantidad = leer_archivo_de_datos((char*) path,
					&nuevos_registros);
			if (cantidad < 0) {
				// Informamos el error y cortamos la iteración
				hubo_error = -1;
				return -1;
			}
			if (_concatenar_registros_al_resultado(nuevos_registros, cantidad)
					< 0) {
				free(nuevos_registros);
				return -1;
			}
			free(nuevos_registros);
		}
		return 0;
	}

	if (iterar_directorio_tabla(tabla, &_leer_archivo) < 0 || hubo_error != 0) {
		free(*resultado);
		return -1;
	}
	return cantidad_actual_registros;
}
