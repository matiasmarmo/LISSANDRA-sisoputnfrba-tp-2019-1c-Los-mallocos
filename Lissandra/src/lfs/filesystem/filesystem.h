#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <stdint.h>
#include <time.h>

#define TAMANIO_PATH 256
#define NOMBRE_DIRECTORIO_TABLAS "Tables/"

enum resultados_operaciones { CONTINUAR, FINALIZAR };

typedef struct registro {
    uint16_t key;
    time_t timestamp;
    char *value;
} registro_t;

typedef int (*operacion_t)(registro_t);

int inicializar_filesystem();

int get_tamanio_bloque();

int get_cantidad_bloques();
#endif
