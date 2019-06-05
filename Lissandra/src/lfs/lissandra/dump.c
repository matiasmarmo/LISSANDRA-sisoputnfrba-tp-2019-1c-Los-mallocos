#include <stdlib.h>

#include <commons/collections/dictionary.h>

#include "../../commons/lissandra-threads.h"

#include "../filesystem/filesystem.h"
#include "../filesystem/manejo-datos.h"
#include "../lfs-config.h"
#include "memtable.h"

t_dictionary *datos_no_dumpeados;
t_dictionary *temporal;

int inicializar_dumper() {
	datos_no_dumpeados = dictionary_create();
	temporal = dictionary_create();
	return datos_no_dumpeados == NULL || temporal == NULL ? -1 : 0;
}

void destruir_dumper() {

	void _destruir(void *elemento) {
		liberar_memtable_data((memtable_data_t*) elemento);
	}

	dictionary_destroy_and_destroy_elements(datos_no_dumpeados, &_destruir);
	dictionary_destroy_and_destroy_elements(temporal, &_destruir);
}

void _iteracion_dump(char *tabla, void *valor, t_dictionary *dict) {
	memtable_data_t *valor_memtable_data = (memtable_data_t*) valor;
	int resultado_bajar;
	resultado_bajar = bajar_a_archivo_temporal(tabla, valor_memtable_data->data,
			valor_memtable_data->cantidad_registros);
	if (resultado_bajar != TABLA_NO_EXISTENTE && resultado_bajar < 0) {
		dictionary_put(dict, tabla, valor);
	} else {
		liberar_memtable_data(valor_memtable_data);
	}
}

void manejar_datos_no_dumpeador(char *tabla, void *valor) {
	_iteracion_dump(tabla, valor, temporal);
}

void bajar_a_archivo_tmp(char *tabla, void *valor) {
	_iteracion_dump(tabla, valor, datos_no_dumpeados);
}

void *dumpear(void *entrada) {
	dictionary_iterator(datos_no_dumpeados, &manejar_datos_no_dumpeador);
	dictionary_destroy(datos_no_dumpeados);
	datos_no_dumpeados = temporal;
	temporal = dictionary_create();
	t_dictionary *diccionario = obtener_datos_para_dumpear();
	if (diccionario == NULL) {
		return NULL;
	}
	dictionary_iterator(diccionario, &bajar_a_archivo_tmp);
	dictionary_destroy(diccionario);
	return NULL;
}

int instanciar_hilo_dumper(lissandra_thread_periodic_t *lp_thread) {
	return l_thread_periodic_create(lp_thread, &dumpear, &get_tiempo_dump, NULL);
}