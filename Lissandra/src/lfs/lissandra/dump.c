#include <stdlib.h>

#include <commons/collections/dictionary.h>

#include "../filesystem/manejo-datos.h"
#include "memtable.h"

void bajar_a_archivo_tmp(char *tabla, void *valor) {
	memtable_data_t *valor_memtable_data = (memtable_data_t*) valor;
	if (bajar_a_archivo_temporal(tabla, valor_memtable_data->data,
			valor_memtable_data->cantidad_registros) < 0) {
	}
}

void *dumpear(void *entrada) {
	t_dictionary *diccionario = obtener_datos_para_dumpear();
	if (diccionario == NULL) {
		return NULL;
	}
	printf("D has TablaA: %d\n", dictionary_has_key(diccionario, "TablaA"));
	dictionary_iterator(diccionario, &bajar_a_archivo_tmp);
	destruir_datos_dumpeados(diccionario);
	return NULL;
}
