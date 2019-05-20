#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdint.h>
#include <time.h>

#define TAMANIO_PATH 256
#define TAMANIO_NOMBRE_ARCHIVO 50
#define NOMBRE_DIRECTORIO_TABLAS "tablas"

enum resultados_operaciones { CONTINUAR, FINALIZAR };

typedef struct registro {
    uint16_t key;
    time_t timestamp;
    char *value;
} registro_t;

typedef int (*operacion_t)(registro_t);

#endif