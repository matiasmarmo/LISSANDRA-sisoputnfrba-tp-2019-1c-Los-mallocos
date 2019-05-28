#ifndef MEMTABLE_H_
#define MEMTABLE_H_

#include <commons/collections/dictionary.h>
#include "../filesystem/filesystem.h"

typedef struct memtable_data {
    registro_t *data;
    int cantidad_registros;
} memtable_data_t;

int inicializar_memtable();

void destruir_memtable();

int insertar_en_memtable(registro_t registro, char *tabla);

int iterar_entrada_memtable(char *tabla, operacion_t operacion);

t_dictionary *obtener_datos_para_dumpear();

void liberar_memtable_data(memtable_data_t *data);

#endif
