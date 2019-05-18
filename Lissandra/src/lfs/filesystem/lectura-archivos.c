#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
#include <sys/file.h>
#include <commons/config.h>

#include "../lfs-config.h"
#include "filesystem.h"
#include "lectura-archivos.h"

#define TIMESTAMP_STR_LEN 20

int _local_get_tamanio_value() {
    // Wrapper para llamar a get_tamanio_value solo una vez
    // y no constantemente cada vez que se itera, así
    // evitamos el overhead de bloquear el semáforo
    // de lfs-config
    static int tamanio = -1;
    if(tamanio == -1) {
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
    if(tamanio == -1) {
        tamanio = _local_get_tamanio_value() + TIMESTAMP_STR_LEN + 5 + 2 + 1;
    }
    return tamanio;
}

int path_a_bloque(char *numero_bloque, char *buffer, int tamanio_buffer) {
    if(tamanio_buffer < TAMANIO_PATH) {
        return -1;
    }
    sprintf(buffer, "%sBloques/%s.bin", get_punto_montaje(), numero_bloque);
    return 0;
}

FILE *_abrir_archivo_con_lock(char *path, char *modo, int tipo_lock) {

    // Abre el archivo con dirección 'path', en el modo de lectura
    // 'modo' ("r", "w", etc) y aplica un lock del tipo 'tipo_lock'.
    // El tipo de lock puede ser:
    //      LOCK_SH: lock compartido (el que usamos para lectura)
    //      LOCK_EX: lock exclusivo (el que usamos para escritura)

    FILE *archivo;
    int file_descriptor;
    if((archivo = fopen(path, modo)) == NULL) {
        return NULL;
    }
    file_descriptor = fileno(archivo);
    if(file_descriptor == -1 || flock(file_descriptor, tipo_lock) < 0) {
        fclose(archivo);
        return NULL;
    }
    return archivo;
}

FILE *abrir_archivo_para_lectura(char *path) {

    // Abre el archivo 'path' para lectura con un lock compartido

    return _abrir_archivo_con_lock(path, "r", LOCK_SH);
}

FILE *abrir_archivo_para_escritura(char *path) {

    // Abre el archivo 'path' para escritura con un lock exclusivo

    return _abrir_archivo_con_lock(path, "w", LOCK_EX);
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
        while(string_registro[indice_string_registro] != ';' && string_registro[indice_string_registro] != '\0') {
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
    if(registro->value == NULL) {
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
    if(fread(&caracter_leido, 1, 1, archivo) != 1) {
        // Retornamos 0 si termino el archivo, -1 ante un error
        return feof(archivo) ? 0 : -1;
    }
    while(caracter_leido != token) {
        buffer_cat[0] = caracter_leido;
        strcat(resultado, buffer_cat);
        if(fread(&caracter_leido, 1, 1, archivo) != 1) {
            // Retornamos 0 si termino el archivo, -1 ante un error
            return feof(archivo) ? 0 : -1;
        }
    }
    return 1;
}

int iterar_bloque(char *path_bloque, char *registro_truncado, operacion_t operacion) {

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
    strcpy(string_registro, registro_truncado);
    memset(registro_truncado, 0, get_tamanio_string_registro());
    if(archivo_bloque == NULL) {
        return -1;
    }

    while((resultado = leer_hasta_siguiente_token(archivo_bloque, string_registro, '\n')) == 1) {
        if(parsear_registro(string_registro, &registro) < 0) {
            resultado = -1;
            break;
        }
        if((resultado = operacion(registro)) != CONTINUAR) {
            break;
        }
        memset(string_registro, 0, get_tamanio_string_registro());
    }

    fclose(archivo_bloque);

    if(resultado == 0) {
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

    t_config *archivo_datos = config_create(path);
    if(archivo_datos == NULL) {
        return -1;
    }

    char registro_truncado[get_tamanio_string_registro()];
    char path_bloque[TAMANIO_PATH] = { 0 };
    int resultado;

    memset(registro_truncado, 0, get_tamanio_string_registro());

    char **bloques = config_get_array_value(archivo_datos, "BLOCKS");
    if(bloques == NULL) {
        config_destroy(archivo_datos);
        return -1;
    }

    for(char **bloque_num = bloques; *bloque_num != NULL; bloque_num++) {
        path_a_bloque(*bloque_num, path_bloque, TAMANIO_PATH);
        resultado = iterar_bloque(path_bloque, registro_truncado, operacion);
        if(resultado != CONTINUAR) {
            break;
        }
    }

    for(char **bloque_num = bloques; *bloque_num != NULL; bloque_num++) {
        free(*bloque_num);
    }
    free(bloques);
    config_destroy(archivo_datos);
    return resultado < 0 ? -1 : 0;
}

int leer_archivo_de_datos(char *path, registro_t **resultado) {

    // Recibe un archivo de datos 'path' y un doble puntero
    // a registro. Aloca memoria al puntero y graba en él
    // todos los registros del archivo de datos.

    // Arrancamos con lugar para 10 registros
    int tamanio_actual = 10 * sizeof(registro_t), registros_cargados = 0;
    bool hubo_error = false;

    *resultado = malloc(tamanio_actual);
    if(*resultado == NULL) {
        return -1;
    }

    int _cargar_registro(registro_t registro) {
        registro_t *puntero_realocado;
        if(registros_cargados >= tamanio_actual) {
            tamanio_actual *= 2;
            puntero_realocado = realloc(*resultado, tamanio_actual);
            if(puntero_realocado == NULL) {
                hubo_error = true;
                return FINALIZAR;
            }
            *resultado = puntero_realocado;
        }
        (*resultado)[registros_cargados++] = registro;
        return CONTINUAR;
    }

    if(iterar_archivo_de_datos(path, &_cargar_registro) < 0 || hubo_error) {
        // Error, liberar resultado
        for(int i = 0; i < registros_cargados; i++) {
            free((*resultado)[i].value);
        }
        free(*resultado);
        return -1;
    }

    return registros_cargados;
}