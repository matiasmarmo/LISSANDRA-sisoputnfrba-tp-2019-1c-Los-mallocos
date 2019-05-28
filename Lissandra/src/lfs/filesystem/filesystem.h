#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdint.h>
#include <time.h>

#define TAMANIO_PATH 256
#define NOMBRE_DIRECTORIO_TABLAS "Tables/"

enum resultados_operaciones { CONTINUAR, FINALIZAR };

enum errores_filesystem { TABLA_NO_EXISTENTE = -20 };

typedef struct registro {
    uint16_t key;
    time_t timestamp;
    char *value;
} registro_t;

typedef int (*operacion_t)(registro_t);

typedef int (*operacion_bloque_t)(int);

int inicializar_filesystem();

int get_tamanio_bloque();

int get_cantidad_bloques();

void obtener_path_particion(int numero, char* nombre_tabla, char* path);

void obtener_path_tabla(char *nombre_tabla, char *path);

void obtener_path_temporal(int numero, char* nombre_tabla, char* path);

void campo_entero_a_string(int entero, char* string);

#endif
