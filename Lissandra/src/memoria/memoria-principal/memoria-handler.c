#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>
#include <sys/time.h>

#include "../../commons/comunicacion/protocol.h"
#include "../../commons/lissandra-threads.h"
#include "memoria-handler.h"
#include "../memoria-logger.h"
#include "../memoria-config.h"
#include "../memoria-server.h"
#include "../memoria-main.h"
#include "../conexion-lfs.h"

#define MAX_TAM_NOMBRE_TABLA 50

pthread_mutex_t mutex_memoria = PTHREAD_MUTEX_INITIALIZER;
uint8_t* memoria;

void bloquear_memoria() {
	pthread_mutex_lock(&mutex_memoria);
}

void desbloquear_memoria() {
	pthread_mutex_unlock(&mutex_memoria);
}

uint8_t* get_memoria() {
	return memoria;
}
int inicializacion_memoria() {
	memoria = calloc(get_tamanio_memoria(), sizeof(char));
	if (memoria == NULL) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Construcción de memoria fallida, error en calloc");
		return -1;
	}
	return 0;
}
void destruccion_memoria() {
	free(memoria);
}

void inicializacion_tabla_segmentos() {
	TABLA_DE_SEGMENTOS = list_create();
}

void destruccion_tabla_registros_paginas() {
	void _destroy_element(void *elemento) {
		free((registro_tabla_pagina*) elemento);
	}
	int cantidad_segmentos = list_size(TABLA_DE_SEGMENTOS);
	segmento* segmento_temporal;
	for (int i = 0; i < cantidad_segmentos; i++) {
		segmento_temporal = list_get(TABLA_DE_SEGMENTOS, i);
		list_destroy_and_destroy_elements(segmento_temporal->registro_base,
				&_destroy_element);
		;
	}
}

void destruccion_tabla_segmentos() {
	void _destroy_element(void *elemento) {
		free((segmento*) elemento);
	}
	list_destroy_and_destroy_elements(TABLA_DE_SEGMENTOS, &_destroy_element);
}

void estado_actual_memoria() {
	int cantidad_segmentos = list_size(TABLA_DE_SEGMENTOS);
	printf("    ______________________________________\n");
	printf("   | Cantidad de segmentos: %d\n", cantidad_segmentos);
	segmento* segmento_temporal;
	for (int i = 0; i < cantidad_segmentos; i++) {
		segmento_temporal = list_get(TABLA_DE_SEGMENTOS, i);
		printf("   |   -Segmento %d --> %d paginas (TABLA: %s)\n", i + 1,
				list_size(segmento_temporal->registro_base),
				segmento_temporal->tabla);
	}
}

uint16_t numero_de_pagina_libre(segmento* segmento) {
	uint16_t cantidad_paginas = list_size(segmento->registro_base);
	uint16_t num_pagina = 1;
	bool _buscar_numero_pagina(void *elemento) {
		registro_tabla_pagina *reg = (registro_tabla_pagina*) elemento;
		return reg->numero_pagina == num_pagina;
	}
	for (num_pagina = 1; num_pagina < cantidad_paginas; num_pagina++) {
		registro_tabla_pagina* reg_pagina = list_find(segmento->registro_base,
				&_buscar_numero_pagina);
		if (reg_pagina == NULL) {
			return num_pagina;
		}
	}
	return cantidad_paginas;
}

void crear_registro_nuevo_en_tabla_de_paginas(int lugar_pagina_vacia,
		segmento* segmento, int flag_modificado,
		struct timeval timestamp_accedido) {
	registro_tabla_pagina* nuevo_registro_pagina = malloc(
			sizeof(registro_tabla_pagina));
	nuevo_registro_pagina->numero_pagina = numero_de_pagina_libre(segmento);
	nuevo_registro_pagina->puntero_a_pagina = memoria + lugar_pagina_vacia;
	nuevo_registro_pagina->flag_modificado = flag_modificado;
	nuevo_registro_pagina->timestamp_accedido.tv_sec =
			timestamp_accedido.tv_sec;
	nuevo_registro_pagina->timestamp_accedido.tv_usec =
			timestamp_accedido.tv_usec;
	list_add(segmento->registro_base, nuevo_registro_pagina);
}

void crear_pagina_nueva(int lugar_pagina_vacia, uint16_t key,
		uint64_t timestamp, char* value) {
	pagina nueva_pagina;
	nueva_pagina.key = (uint16_t*) (memoria + 0 + lugar_pagina_vacia);
	nueva_pagina.timestamp = (uint64_t*) (memoria + sizeof(uint16_t)
			+ lugar_pagina_vacia);
	nueva_pagina.value = (char*) (memoria + sizeof(uint16_t) + sizeof(uint64_t)
			+ lugar_pagina_vacia);
	*(nueva_pagina.key) = key;
	*(nueva_pagina.timestamp) = timestamp;
	strcpy(nueva_pagina.value, value);
}

void crear_segmento_nuevo(char* nombre_tabla_nueva) {
	segmento* nuevo_segmento = malloc(sizeof(segmento));
	strcpy(nuevo_segmento->tabla, nombre_tabla_nueva);
	nuevo_segmento->registro_base = list_create();
	nuevo_segmento->registro_limite = 0;
	list_add(TABLA_DE_SEGMENTOS, nuevo_segmento);
}

registro_tabla_pagina* encontrar_pagina_en_memoria(segmento* segmento,
		uint16_t key) {
	bool _buscar_pagina_en_memoria(void *elemento) {
		registro_tabla_pagina *reg = (registro_tabla_pagina*) elemento;
		pagina final;
		final.key = (uint16_t*) (reg->puntero_a_pagina);
		return *(final.key) == key;
	}
	registro_tabla_pagina* reg_pagina = list_find(segmento->registro_base,
			&_buscar_pagina_en_memoria);
	return reg_pagina;
}

segmento* encontrar_segmento_en_memoria(char* nombre_tabla_buscada) {
	bool _buscar_segmento_en_memoria(void *elemento) {
		segmento *b = (segmento*) elemento;
		return !strcmp(b->tabla, nombre_tabla_buscada);
	}
	segmento *segmento = list_find(TABLA_DE_SEGMENTOS,
			&_buscar_segmento_en_memoria);
	return segmento;
}

int encontrar_pagina_vacia() {
	int numero_pagina = 0;
	while (get_tamanio_memoria() >= get_tamanio_maximo_pagina() + numero_pagina) {
		if (*(memoria + numero_pagina) == 0) {
			return numero_pagina;
		}
		numero_pagina += get_tamanio_maximo_pagina();
	}
	return -1;
}

int cantidad_paginas_de_un_segmento(segmento* segmento) {
	int cantidad = list_size(segmento->registro_base);
	return cantidad;
}

void setear_pagina_a_cero(uint8_t *puntero_a_pagina) {
	memset(puntero_a_pagina, 0, get_tamanio_maximo_pagina());
}

void destruir_registro_de_pagina(registro_tabla_pagina *registro) {
	setear_pagina_a_cero(registro->puntero_a_pagina);
	free(registro);
}

int obtener_pagina_para_journal(segmento* segmento,
		registro_tabla_pagina* reg_pagina) {
	struct insert_request request;
	uint8_t *puntero_a_pagina = reg_pagina->puntero_a_pagina;
	init_insert_request(segmento->tabla,
			*((uint16_t*) puntero_a_pagina),
			(char*) (puntero_a_pagina + 10),
			*((uint64_t*) (puntero_a_pagina + 2)), &request);
	uint16_t key = *((uint16_t*) puntero_a_pagina);
	uint8_t buffer[get_max_msg_size()];
	if (enviar_mensaje_lfs(&request, buffer, false) < 0) {
		memoria_log_to_level(LOG_LEVEL_ERROR, false,
				"Fallo la comunicacion con file system, key %d perdida", key);
		destroy(&request);
		return -1;
	}
	destroy(&request);
	// Acá se puede verificar si falló el insert y loguear
	// (si no existe la tabla, por ejemplo)
	destroy(buffer);
	return 0;
}
void imprimir_toda_memoria();
int realizar_journal() {
	segmento* segmento_temporal;
	int contador=0;
	void _liberar_pagina(void *elemento) {
		destruir_registro_de_pagina((registro_tabla_pagina*) elemento);
	}

	for(int i = 0; i < list_size(TABLA_DE_SEGMENTOS); i++) {
		segmento_temporal = list_get(TABLA_DE_SEGMENTOS, i);
		int cantidad_paginas = cantidad_paginas_de_un_segmento(list_get(TABLA_DE_SEGMENTOS, i));
		for(int j = 0; j < cantidad_paginas; j++) {
			registro_tabla_pagina *pagina_actual = list_get(segmento_temporal->registro_base, j);
			if(pagina_actual->flag_modificado == 0) {
				// Si la página no está modificada, no la mandamos
				continue;
			}
			if(obtener_pagina_para_journal(segmento_temporal, pagina_actual) < 0) {
				// Acá había un return -1, pero lo cambio por un continue
				// porque es peligroso dejar las cosas hechas por la mitad,
				// obtener_pagina_para_journal loguea el error y listo
				continue;
			}
			contador++;
		}
		list_destroy_and_destroy_elements(segmento_temporal->registro_base, &_liberar_pagina);
	}
	list_clean_and_destroy_elements(TABLA_DE_SEGMENTOS, free);
	memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Se realizó el journal correctamente. Cantidad paginas enviadas: %d",contador);
	return 0;
}

void *realizar_journal_threaded(void *entrada) {
	if (pthread_mutex_lock(&mutex_memoria) != 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al bloquear semáforo en journal");
		return NULL;
	}
	realizar_journal();
	pthread_mutex_unlock(&mutex_memoria);
	return NULL;
}

void destruccion_todos_los_registros_de_un_segmento(segmento* segmento) {
	void _destroy_element(void *elemento) {
		setear_pagina_a_cero(((registro_tabla_pagina*) elemento)->puntero_a_pagina);
		free((registro_tabla_pagina*) elemento);
	}
	list_destroy_and_destroy_elements(segmento->registro_base,
			&_destroy_element);
}

void destruccion_segmento(segmento* segmento) {
	destruccion_todos_los_registros_de_un_segmento(segmento);
	free(segmento);
}

void iterar_tabla_de_segmentos(void (*f)(void*)) {
	list_iterate(TABLA_DE_SEGMENTOS, f);
}