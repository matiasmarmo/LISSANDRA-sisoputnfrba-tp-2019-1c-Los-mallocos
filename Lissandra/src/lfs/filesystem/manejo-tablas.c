#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <ftw.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>

#include "../../commons/comunicacion/protocol.h"
#include "../compactador.h"
#include "../lfs-config.h"
#include "manejo-tablas.h"
#include "filesystem.h"
#include "bitmap.h"
#include "config-with-locks.h"
#include "manejo-datos.h"
#include "../lfs-logger.h"

pthread_rwlock_t semaforo_dict_bloque = PTHREAD_RWLOCK_INITIALIZER;
t_dictionary *diccionario_bloqueo_tablas;

int bloquear_tabla(char *nombre_tabla, char forma) {
	pthread_rwlock_wrlock(&semaforo_dict_bloque);
	if (!dictionary_has_key(diccionario_bloqueo_tablas, nombre_tabla)) {
		pthread_rwlock_unlock(&semaforo_dict_bloque);
		return -1;
	}
	pthread_rwlock_t *lock = dictionary_get(diccionario_bloqueo_tablas,
			nombre_tabla);
	if (forma == 'w') {
		pthread_rwlock_wrlock(lock);
	} else if (forma == 'r') {
		pthread_rwlock_rdlock(lock);
	}
	pthread_rwlock_unlock(&semaforo_dict_bloque);
	return 0;
}

int desbloquear_tabla(char *nombre_tabla) {
	pthread_rwlock_wrlock(&semaforo_dict_bloque);
	if (!dictionary_has_key(diccionario_bloqueo_tablas, nombre_tabla)) {
		pthread_rwlock_unlock(&semaforo_dict_bloque);
		return -1;
	}
	pthread_rwlock_t *lock = dictionary_get(diccionario_bloqueo_tablas,
			nombre_tabla);
	pthread_rwlock_unlock(lock);
	pthread_rwlock_unlock(&semaforo_dict_bloque);
	return 0;
}

int crear_semaforo_tabla(char *nombre_tabla) {
	pthread_rwlock_wrlock(&semaforo_dict_bloque);
	pthread_rwlock_t *nuevo_lock = malloc(sizeof(pthread_rwlock_t));
	if (nuevo_lock == NULL) {
		pthread_rwlock_unlock(&semaforo_dict_bloque);
		return -1;
	}
	if (pthread_rwlock_init(nuevo_lock, NULL) != 0) {
		pthread_rwlock_unlock(&semaforo_dict_bloque);
		free(nuevo_lock);
		return -1;
	}
	dictionary_put(diccionario_bloqueo_tablas, nombre_tabla, nuevo_lock);
	pthread_rwlock_unlock(&semaforo_dict_bloque);
	return 0;
}

int destruir_semaforo_tabla(char *nombre_tabla) {
	pthread_rwlock_wrlock(&semaforo_dict_bloque);
	pthread_rwlock_t *lock = dictionary_remove(diccionario_bloqueo_tablas, nombre_tabla);
	if(lock != NULL) {
		pthread_rwlock_wrlock(lock);
		pthread_rwlock_unlock(lock);
		pthread_rwlock_destroy(lock);
		free(lock);
	}
	pthread_rwlock_unlock(&semaforo_dict_bloque);
	return 0;
}

int crear_directorio_tabla(char* nombre_tabla) {
	char path_tabla[TAMANIO_PATH];

	obtener_path_tabla(nombre_tabla, path_tabla);
	if (mkdir(path_tabla,
	S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
			== -1) {
		return -1;
	}
	return 0;
}

int crear_metadata_tabla(metadata_t metadata, char* nombre_tabla) {
	char escritura[200];
	char consistencia_string[10];
	char n_particiones_string[15];
	char t_compactaciones_string[15];
	char nombre_metadata[TAMANIO_PATH];
	char path_tabla[TAMANIO_PATH];
	FILE* metadata_tabla_f;

	obtener_path_tabla(nombre_tabla, path_tabla);
	strcpy(nombre_metadata, path_tabla);
	strcat(nombre_metadata, "/");
	strcat(nombre_metadata, nombre_tabla);
	strcat(nombre_metadata, "-metadata");
	strcat(nombre_metadata, ".bin");
	if ((metadata_tabla_f = abrir_archivo_para_escritura(nombre_metadata))
			== NULL) {
		return -1;
	}

	strcpy(escritura, "CONSISTENCY=");
	campo_entero_a_string(metadata.consistencia, consistencia_string);
	strcat(escritura, consistencia_string);
	strcat(escritura, "\nPARTITIONS=");
	campo_entero_a_string(metadata.n_particiones, n_particiones_string);
	strcat(escritura, n_particiones_string);
	strcat(escritura, "\nCOMPACTION_TIME=");
	campo_entero_a_string(metadata.t_compactaciones, t_compactaciones_string);
	strcat(escritura, t_compactaciones_string);

	if (fwrite(escritura, strlen(escritura), 1, metadata_tabla_f) < 0) {
		fclose(metadata_tabla_f);
		return -1;
	}

	fclose(metadata_tabla_f);
	return 0;
}

int crear_particion(int numero, char* nombre_tabla) {
	FILE* binario_f;
	char nombre_binario[TAMANIO_PATH] = { 0 }, escritura[200] = { 0 };
	char block_string[10];
	int bloque;

	bloque = pedir_bloque_bitmap();
	if (bloque < 0) {
		return -1;
	}
	obtener_path_particion(numero, nombre_tabla, nombre_binario);

	if ((binario_f = abrir_archivo_para_escritura(nombre_binario)) == NULL) {
		liberar_bloque_bitmap(bloque);
		return -1;
	}

	strcpy(escritura, "SIZE=0");
	strcat(escritura, "\nBLOCKS=[");
	campo_entero_a_string(bloque, block_string);
	strcat(escritura, block_string);
	strcat(escritura, "]");

	if (fwrite(escritura, strlen(escritura), 1, binario_f) < 0) {
		fclose(binario_f);
		return -1;
	}
	fclose(binario_f);

	return 0;
}

int crear_temporal(int numero, char* nombre_tabla) {
	FILE* tmp_f;
	char nombre_tmp[TAMANIO_PATH] = { 0 }, escritura[200] = { 0 };
	char block_string[10];
	int bloque;

	bloque = pedir_bloque_bitmap();
	if (bloque < 0) {
		return -1;
	}
	obtener_path_temporal(numero, nombre_tabla, nombre_tmp);

	if ((tmp_f = abrir_archivo_para_escritura(nombre_tmp)) == NULL) {
		liberar_bloque_bitmap(bloque);
		return -1;
	}

	strcpy(escritura, "SIZE=0");
	strcat(escritura, "\nBLOCKS=[");
	campo_entero_a_string(bloque, block_string);
	strcat(escritura, block_string);
	strcat(escritura, "]");

	if (fwrite(escritura, strlen(escritura), 1, tmp_f) < 0) {
		fclose(tmp_f);
		return -1;
	}
	fclose(tmp_f);

	return 0;
}

int crear_particiones(int n_particiones, char* nombre_tabla) {

	for (int numero = 0; numero < n_particiones; numero++) {
		if (crear_particion(numero, nombre_tabla) < 0) {
			return -1;
		}
	}

	return 0;
}

int crear_tabla(char* nombre_tabla, metadata_t metadata) {

	if (crear_directorio_tabla(nombre_tabla) == -1) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear la tabla %s", nombre_tabla);
		return -1;
	}
	if (crear_metadata_tabla(metadata, nombre_tabla) == -1) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear la metadata de la tabla %s", nombre_tabla);
		borrar_tabla(nombre_tabla);
		return -1;
	}
	if (crear_particiones(metadata.n_particiones, nombre_tabla) == -1) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear las particiones de la tabla %s", nombre_tabla);
		borrar_tabla(nombre_tabla);
		return -1;
	}
	if(crear_semaforo_tabla(nombre_tabla) < 0) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear la tabla %s", nombre_tabla);
		borrar_tabla(nombre_tabla);
		return -1;
	}
	if (instanciar_hilo_compactador(nombre_tabla, metadata.t_compactaciones)
			< 0) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear la tabla %s", nombre_tabla);
		destruir_semaforo_tabla(nombre_tabla);
		borrar_tabla(nombre_tabla);
		return -1;
	}
	return 0;
}

int existe_tabla(char* nombre_tabla) {
	char path_tabla[TAMANIO_PATH] = { 0 };
	struct stat buffer;
	obtener_path_tabla(nombre_tabla, path_tabla);
	if (stat(path_tabla, &buffer) < 0) {
		if (errno == ENOENT) {
			return 1;
		}
		return -1;
	}
	return 0;
}

int obtener_metadata_tabla(char* nombre_tabla, metadata_t* metadata_tabla) {
	char buffer[TAMANIO_PATH];

	obtener_path_tabla(nombre_tabla, buffer);
	strcat(buffer, nombre_tabla);
	strcat(buffer, "-metadata.bin");
	FILE *file_metadata = abrir_archivo_para_lectura(buffer);
	if(file_metadata == NULL) {
		return -1;
	}
	t_config* metadata_config_tabla = lfs_config_create_from_file(buffer, file_metadata);
	fclose(file_metadata);

	if (metadata_config_tabla == NULL) {
		return -1;
	}

	metadata_tabla->consistencia = config_get_int_value(metadata_config_tabla,
			"CONSISTENCY");
	metadata_tabla->n_particiones = config_get_int_value(metadata_config_tabla,
			"PARTITIONS");
	metadata_tabla->t_compactaciones = config_get_int_value(
			metadata_config_tabla, "COMPACTION_TIME");

	config_destroy(metadata_config_tabla);

	return 0;
}

int iterar_directorio_tabla(char *tabla,
		int (funcion)(const char*, const struct stat*, int)) {

	char path_tabla[TAMANIO_PATH] = { 0 };
	obtener_path_tabla(tabla, path_tabla);
	return ftw(path_tabla, funcion, 10);

}

int borrar_tabla(char *tabla) {

	int _borrar_archivo(const char *path, const struct stat *stat, int flag) {
		if (string_ends_with((char*) path, ".bin")
				&& !string_ends_with((char*) path, "metadata.bin")) {
			FILE *archivo = abrir_archivo_para_lectoescritura((char*) path);
			if (archivo == NULL) {
				// Retornamos 0 porque ftw corta la iteración en otro caso
				return 0;
			}
			t_config *config = lfs_config_create_from_file((char*) path,
					archivo);
			if (config == NULL) {
				fclose(archivo);
				return 0;
			}
			liberar_bloques_de_archivo(config);
			config_destroy(config);
			fclose(archivo);
		}
		remove(path);
		return 0;
	}

	pthread_rwlock_wrlock(&semaforo_dict_bloque);
	pthread_rwlock_t *lock = dictionary_remove(diccionario_bloqueo_tablas, tabla);
	if(lock != NULL) {
		pthread_rwlock_wrlock(lock);
		pthread_rwlock_unlock(lock);
		pthread_rwlock_destroy(lock);
		free(lock);
	}

	char path_tabla[TAMANIO_PATH] = { 0 };
	obtener_path_tabla(tabla, path_tabla);
	finalizar_hilo_compactador(tabla);
	iterar_directorio_tabla(tabla, &_borrar_archivo);
	int res = rmdir(path_tabla);
	pthread_rwlock_unlock(&semaforo_dict_bloque);
	return res;
}

void borrar_todos_los_tmpc(char *tabla) {

	// No hace falta porque si existe el compactador, existe la tabla y esta
	// funcion es llamada solo por el compactador
	// Lo dejamos conectado por las dudas
//	if (existe_tabla(tabla) != 0) {
//		lfs_log_to_level(LOG_LEVEL_ERROR, false, "No existe la tabla %s", tabla);
//		return;
//	}

	int _borrar_archivo(const char *path, const struct stat *stat, int flag) {
		if (string_ends_with((char*) path, ".tmpc")
				|| string_ends_with((char*) path, ".tmpc/")) {
			FILE *archivo = abrir_archivo_para_lectoescritura((char*) path);
			if (archivo == NULL) {
				// Retornamos 0 porque ftw corta la iteración en otro caso
				return 0;
			}
			t_config *config = lfs_config_create_from_file((char*) path,
					archivo);
			if (config == NULL) {
				fclose(archivo);
				return 0;
			}
			liberar_bloques_de_archivo(config);
			config_destroy(config);
			fclose(archivo);
			remove(path);
		}
		return 0;
	}

	iterar_directorio_tabla(tabla, &_borrar_archivo);
}

void convertir_todos_tmp_a_tmpc(char* tabla) {

	int _renombrar_archivo_a_tmpc(const char *path, const struct stat *stat,
			int flag) {
		if (string_ends_with((char*) path, ".tmp")
				|| string_ends_with((char*) path, ".tmp/")) {

			char path_nuevo_temp[TAMANIO_PATH];
			strcpy(path_nuevo_temp, path);
			strcat(path_nuevo_temp, "c");
			rename(path, path_nuevo_temp);
		}
		return 0;
	}
	iterar_directorio_tabla(tabla, &_renombrar_archivo_a_tmpc);
}

int dar_metadata_tablas(t_list* nombre_tablas, t_list* metadatas) {
	int hubo_error = 0;
	DIR *directorio;
	struct dirent *directorio_datos;

	char directorio_path[TAMANIO_PATH];
	char* punto_montaje = get_punto_montaje();
	sprintf(directorio_path, "%sTables/", punto_montaje);

	directorio = opendir(directorio_path);

	if (directorio == NULL) {
		return -1;
	}

	metadata_t *metadata_temp;
	char *nombre_temp;
	while ((directorio_datos = readdir(directorio))) {
		if (string_starts_with(directorio_datos->d_name, ".")) {
			continue;
		}
		if ((nombre_temp = strdup(directorio_datos->d_name)) == NULL) {
			hubo_error = 1;
			break;
		}
		if ((metadata_temp = malloc(sizeof(metadata_t))) == NULL) {
			free(nombre_temp);
			hubo_error = 1;
			break;
		}
		if (obtener_metadata_tabla(directorio_datos->d_name, metadata_temp)
				< 0) {
			free(nombre_temp);
			free(metadata_temp);
			hubo_error = 1;
			break;
		}
		list_add(nombre_tablas, nombre_temp);
		list_add(metadatas, metadata_temp);
	}

	closedir(directorio);

	void _destruir(void *elemento) {
		free(elemento);
	}

	if (hubo_error) {
		list_clean_and_destroy_elements(nombre_tablas, &_destruir);
		list_clean_and_destroy_elements(metadatas, &_destruir);
		return -1;
	}

	return 0;
}

void obtener_campos_metadatas(uint8_t* consistencias,
		uint8_t* numeros_particiones, uint32_t* tiempos_compactaciones,
		t_list* metadatas) {
	metadata_t* metadata;

	for (int i = 0; i < list_size(metadatas); i++) {
		metadata = (metadata_t*) list_get(metadatas, i);
		consistencias[i] = metadata->consistencia;
		numeros_particiones[i] = metadata->n_particiones;
		tiempos_compactaciones[i] = metadata->t_compactaciones;
	}
}

int cantidad_tmp_en_tabla(char *nombre_tabla) {
	int numero = 0;

	int _cantidad_tmp_en_tabla(const char* path_tmp, const struct stat* stat,
			int flag) {
		if (string_ends_with((char*) path_tmp, ".tmp")
				|| string_ends_with((char*) path_tmp, ".tmp/")) {
			numero++;
		}
		return 0;
	}

	bloquear_tabla(nombre_tabla, 'r');
	if (iterar_directorio_tabla(nombre_tabla, &_cantidad_tmp_en_tabla) < 0) {
		desbloquear_tabla(nombre_tabla);
		return -1;
	}

	desbloquear_tabla(nombre_tabla);
	return numero;
}

void crear_diccionario_bloqueo() {
	diccionario_bloqueo_tablas = dictionary_create();
}

void destruir_diccionario_bloqueo() {
	pthread_rwlock_wrlock(&semaforo_dict_bloque);
	void _destruir_lock(void *lock_cast) {
		pthread_rwlock_t *lock = (pthread_rwlock_t*) lock_cast;
		pthread_rwlock_destroy(lock);
		free(lock);
	}
	dictionary_destroy_and_destroy_elements(diccionario_bloqueo_tablas,
			&_destruir_lock);
	pthread_rwlock_unlock(&semaforo_dict_bloque);
}
