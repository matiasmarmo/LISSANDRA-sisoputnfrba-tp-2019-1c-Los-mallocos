#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/collections/dictionary.h>

#include "../commons/lissandra-threads.h"
#include "filesystem/filesystem.h"
#include "filesystem/manejo-tablas.h"
#include "filesystem/manejo-datos.h"
#include "lfs-logger.h"

pthread_mutex_t hilos_compactadores_mutex = PTHREAD_MUTEX_INITIALIZER;
t_dictionary *hilos_compactadores;

int inicializar_compactador() {
	if (pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
		return -1;
	}
	hilos_compactadores = dictionary_create();
	pthread_mutex_unlock(&hilos_compactadores_mutex);
	return hilos_compactadores == NULL ? -1 : 0;
}

void destruir_hilo_compactador(void *hilo) {
	lissandra_thread_periodic_t *lp_thread = (lissandra_thread_periodic_t*) hilo;
	l_thread_solicitar_finalizacion(&lp_thread->l_thread);
	l_thread_join(&lp_thread->l_thread, NULL);
	free(lp_thread->l_thread.entrada);
	free(hilo);
}

int destruir_compactador() {
	if (pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
		return -1;
	}
	dictionary_destroy_and_destroy_elements(hilos_compactadores,
			&destruir_hilo_compactador);
	pthread_mutex_unlock(&hilos_compactadores_mutex);
	return 0;
}

void liberar_array_registros(registro_t *array, int cantidad) {
	for (int i = 0; i < cantidad; i++) {
		free(array[i].value);
		array[i].value = NULL;
	}
	free(array);
}

int agregar_registro_a_particion(registro_t **registros, int *cant_actual,
		int *maximo, registro_t nuevo_registro) {
	for (int i = 0; i < *cant_actual; i++) {
		if ((*registros)[i].key == nuevo_registro.key) {
			if ((*registros)[i].timestamp < nuevo_registro.timestamp) {
				free((*registros)[i].value);
				(*registros)[i] = nuevo_registro;
			} else {
				free(nuevo_registro.value);
			}
			return 0;
		}
	}
	if (*cant_actual >= *maximo) {
		*maximo = *maximo == 0 ? 10 : *maximo * 2;
		registro_t *nuevos_registros = realloc(*registros,
				*maximo * sizeof(registro_t));
		if (nuevos_registros == NULL) {
			return -1;
		}
		*registros = nuevos_registros;
	}
	(*registros)[*cant_actual] = nuevo_registro;
	(*cant_actual)++;
	return 0;
}

void compactar_particion(char *nombre_tabla, int nro_particion,
		int total_particiones, registro_t *datos_tmpc, int cantidad) {

	registro_t *datos_particion;
	int ret;
	int cantidad_registros_particion = leer_particion(nombre_tabla,
			nro_particion, &datos_particion);
	int tamanio_buffer_particion = cantidad_registros_particion;
	if (cantidad_registros_particion < 0) {
		// log
		return;
	}
	for (int i = 0; i < cantidad; i++) {
		if (datos_tmpc[i].key % total_particiones == nro_particion) {
			ret = agregar_registro_a_particion(&datos_particion,
					&cantidad_registros_particion, &tamanio_buffer_particion,
					datos_tmpc[i]);
			if (ret < 0) {
				liberar_array_registros(datos_particion,
						cantidad_registros_particion);
				// log
				return;
			}
		}
	}
	if (pisar_particion(nombre_tabla, nro_particion, datos_particion,
			cantidad_registros_particion) < 0) {
		// log
	}
	liberar_array_registros(datos_particion, cantidad_registros_particion);
}

int compactar(char *nombre_tabla) {
	static int metadata_cargada = 0;
	static metadata_t metadata;

	if (!metadata_cargada) {
		if (obtener_metadata_tabla(nombre_tabla, &metadata) < 0) {
			// log
			return -1;
		}
		metadata_cargada = 1;
	}
	bloquear_tabla(nombre_tabla, 'w');
	registro_t *datos_tmpc;
	convertir_todos_tmp_a_tmpc(nombre_tabla);
	int cantidad_registros_tmpc = obtener_datos_de_tmpcs(nombre_tabla,
			&datos_tmpc);
	if (cantidad_registros_tmpc < 0) {
		// log
		desbloquear_tabla(nombre_tabla);
		return -1;
	}

	for (int particion = 0; particion < metadata.n_particiones; particion++) {
		compactar_particion(nombre_tabla, particion, metadata.n_particiones,
				datos_tmpc, cantidad_registros_tmpc);
	}

	free(datos_tmpc);
	borrar_todos_los_tmpc(nombre_tabla);
	desbloquear_tabla(nombre_tabla);
	return 0;
}

void *thread_compactacion(void *data) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) data;
	char *nombre_tabla = (char*) l_thread->entrada;
	compactar(nombre_tabla);
	return NULL;
}

int instanciar_hilo_compactador(char *tabla, int t_compactaciones) {
	lissandra_thread_periodic_t *lp_thread;
	char *entrada_thread;
	if (pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
		return -1;
	}
	if (dictionary_has_key(hilos_compactadores, tabla)) {
		pthread_mutex_unlock(&hilos_compactadores_mutex);
		return -1;
	}
	entrada_thread = strdup(tabla);
	if (entrada_thread == NULL) {
		pthread_mutex_unlock(&hilos_compactadores_mutex);
		return -1;
	}
	lp_thread = malloc(sizeof(lissandra_thread_periodic_t));
	if (lp_thread == NULL) {
		pthread_mutex_unlock(&hilos_compactadores_mutex);
		free(entrada_thread);
		return -1;
	}
	if (l_thread_periodic_create_fixed(lp_thread, &thread_compactacion,
			t_compactaciones, entrada_thread) < 0) {
		pthread_mutex_unlock(&hilos_compactadores_mutex);
		free(lp_thread);
		free(entrada_thread);
		return -1;
	}
	dictionary_put(hilos_compactadores, tabla, lp_thread);
	pthread_mutex_unlock(&hilos_compactadores_mutex);
	return 0;
}

int finalizar_hilo_compactador(char *tabla) {
	if (pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
		return -1;
	}
	if (!dictionary_has_key(hilos_compactadores, tabla)) {
		pthread_mutex_unlock(&hilos_compactadores_mutex);
		return -1;
	}
	void *hilo_compactador = dictionary_remove(hilos_compactadores, tabla);
	destruir_hilo_compactador(hilo_compactador);
	pthread_mutex_unlock(&hilos_compactadores_mutex);
	return 0;
}

void compactar_todas_las_tablas() {

	int resultado = 0;

	void _compactar(char *tabla, void *valor) {
		int res_compactacion = compactar(tabla);
		resultado = resultado || res_compactacion;
	}

	pthread_mutex_lock(&hilos_compactadores_mutex);
	dictionary_iterator(hilos_compactadores, &_compactar);
	pthread_mutex_unlock(&hilos_compactadores_mutex);
}
