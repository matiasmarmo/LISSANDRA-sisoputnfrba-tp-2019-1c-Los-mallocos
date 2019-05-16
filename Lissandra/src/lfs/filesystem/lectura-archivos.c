#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
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

int parsear_registro(char *string_registro, registro_t *registro) {
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
    registro->key = (uint16_t) strtoumax(buffer, NULL, 10);
    _siguiente_substring();
    registro->timestamp = (time_t) strtoumax(buffer, NULL, 10);
    _siguiente_substring();
    registro->value = malloc(strlen(buffer) + 1);
    if(registro->value == NULL) {
        return -1;
    }
    strcpy(registro->value, buffer);
    return 0;
}

int leer_hasta_siguiente_token(FILE *archivo, char *resultado, char token) {
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
    FILE *archivo_bloque = fopen(path_bloque, "r");
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