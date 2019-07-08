#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <commons/collections/list.h>
#include <commons/string.h>

#include "../../commons/comunicacion/protocol.h"
#include "../kernel-logger.h"
#include "manager-memorias.h"
#include "metadata-tablas.h"

#define MAX_NOMBRE_TABLA_SIZE 80

typedef struct metadata_tabla {
	char nombre_tabla[MAX_NOMBRE_TABLA_SIZE];
	uint8_t consistencia;
} metadata_tabla_t;

pthread_rwlock_t tablas_rwlock = PTHREAD_RWLOCK_INITIALIZER;
t_list *tablas;

metadata_tabla_t *construir_tabla(char *nombre, uint8_t consistencia) {
	metadata_tabla_t *tabla = malloc(sizeof(metadata_tabla_t));
	if (tabla == NULL) {
		return NULL;
	}
	strcpy(tabla->nombre_tabla, nombre);
	tabla->consistencia = consistencia;
	return tabla;
}

void destruir_tabla(metadata_tabla_t *tabla) {
	free(tabla);
}

metadata_tabla_t *buscar_tabla(char *nombre) {

	bool _tabla_encontrada(void *elemento) {
		metadata_tabla_t *tabla = (metadata_tabla_t*) elemento;
		return strcmp(tabla->nombre_tabla, nombre) == 0;
	}
	return (metadata_tabla_t*) list_find(tablas, &_tabla_encontrada);
}

int cargar_tabla(metadata_tabla_t *nueva_tabla) {
	metadata_tabla_t *tabla_ya_cargada = buscar_tabla(
			nueva_tabla->nombre_tabla);
	if (tabla_ya_cargada == NULL) {
		list_add(tablas, nueva_tabla);
	} else {
		destruir_tabla(nueva_tabla);
	}
	return 0;
}

int cargar_tablas(struct global_describe_response response) {
	char **tablas_recibidas;
	if (pthread_rwlock_wrlock(&tablas_rwlock) != 0) {
		return -1;
	}
	if ((tablas_recibidas = string_split(response.tablas, ";")) == NULL) {
		destroy(&response);
		pthread_rwlock_unlock(&tablas_rwlock);
		return -1;
	}
	for (int i = 0; i < response.consistencias_len; i++) {
		metadata_tabla_t *nueva_tabla = construir_tabla(tablas_recibidas[i],
				response.consistencias[i]);
		free(tablas_recibidas[i]);
		if (nueva_tabla == NULL) {
			continue;
		}
		cargar_tabla(nueva_tabla);
	}
	free(tablas_recibidas);
	pthread_rwlock_unlock(&tablas_rwlock);
	return 0;
}

int pedir_tablas_a_memoria() {
	struct global_describe_response response;
	if (realizar_describe(&response) < 0) {
		kernel_log_to_level(LOG_LEVEL_WARNING, false, "Error al pedir metadata de tablas");
		return -1;
	}
	cargar_tablas(response);
	destroy(&response);
	return 0;
}

int inicializar_tablas() {
	tablas = list_create();
	if (pedir_tablas_a_memoria() < 0) {
		return -1;
	}
	return 0;
}

int destruir_tablas() {
	if (pthread_rwlock_wrlock(&tablas_rwlock) != 0) {
		return -1;
	}

	void _destruir(void *elemento) {
		destruir_tabla((metadata_tabla_t*) elemento);
	}

	list_destroy_and_destroy_elements(tablas, &_destruir);
	pthread_rwlock_unlock(&tablas_rwlock);
	return 0;
}

int obtener_metadata_tablas() {
	return pedir_tablas_a_memoria();
}

void *actualizar_tablas_threaded(void *entrada) {
	obtener_metadata_tablas();
	return NULL;
}

int obtener_consistencia_tabla(char *nombre) {
	metadata_tabla_t *tabla;
	if (pthread_rwlock_rdlock(&tablas_rwlock) != 0) {
		return -1;
	}
	if ((tabla = buscar_tabla(nombre)) == NULL) {
		pthread_rwlock_unlock(&tablas_rwlock);
		return -1;
	}
	pthread_rwlock_unlock(&tablas_rwlock);
	return tabla->consistencia;
}
