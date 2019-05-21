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

t_dictionary *obtener_datos_para_dumpear();

void destruir_datos_dumpeados(t_dictionary *datos);

#endif