#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <commons/collections/list.h>

#include "metricas.h"

typedef struct load_memoria {
	uint16_t id_memoria;
	int reads_realizadas;
	int writes_realizadas;
} load_memoria_t;

typedef struct operacion {
	tipo_operacion_t tipo;
	uint32_t latency;
	uint16_t id_memoria;
	time_t timestamp;
} operacion_t;

typedef struct metrica_criterio {
	int reads_totales;
	int writes_totales;
	t_list* reads;
	t_list* writes;
	t_list* loads;
} metrica_criterio_t;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

metrica_criterio_t metrica_sc;
metrica_criterio_t metrica_shc;
metrica_criterio_t metrica_ec;

void inicializar_metrica(metrica_criterio_t *metrica) {
	metrica->reads_totales = 0;
	metrica->writes_totales = 0;
	metrica->reads = list_create();
	metrica->writes = list_create();
	metrica->loads = list_create();
}

int inicializar_metricas() {
	int error;
	if ((error = pthread_mutex_lock(&mutex)) != 0) {
		return -error;
	}
	inicializar_metrica(&metrica_sc);
	inicializar_metrica(&metrica_shc);
	inicializar_metrica(&metrica_ec);
	pthread_mutex_unlock(&mutex);
	return 0;
}

void destruir_metrica(metrica_criterio_t metrica) {

	void _destruir_elemento(void *elemento) {
		free(elemento);
	}

	list_destroy_and_destroy_elements(metrica.reads, &_destruir_elemento);
	list_destroy_and_destroy_elements(metrica.writes, &_destruir_elemento);
	list_destroy_and_destroy_elements(metrica.loads, &_destruir_elemento);
}

int destruir_metricas() {
	int error;
	if ((error = pthread_mutex_lock(&mutex)) != 0) {
		return -error;
	}
	destruir_metrica(metrica_sc);
	destruir_metrica(metrica_shc);
	destruir_metrica(metrica_ec);
	pthread_mutex_unlock(&mutex);
	return 0;
}

operacion_t *construir_operacion_ptr(tipo_operacion_t tipo, uint32_t latency,
		uint16_t id_memoria) {
	operacion_t *operacion_ptr = malloc(sizeof(operacion_t));
	time_t timestamp = time(NULL);
	if (operacion_ptr == NULL) {
		return NULL;
	}
	if (timestamp == -1) {
		free(operacion_ptr);
		return NULL;
	}
	operacion_ptr->tipo = tipo;
	operacion_ptr->id_memoria = id_memoria;
	operacion_ptr->latency = latency;
	operacion_ptr->timestamp = timestamp;
	return operacion_ptr;
}

load_memoria_t *construir_load_memoria(metrica_criterio_t *metrica,
		uint16_t id_memoria) {
	load_memoria_t *load = malloc(sizeof(load_memoria_t));
	if (load == NULL) {
		return NULL;
	}
	load->id_memoria = id_memoria;
	load->reads_realizadas = 0;
	load->writes_realizadas = 0;
	list_add(metrica->loads, load);
	return load;
}

int registrar_operacion_en_criterio(operacion_t *operacion_ptr,
		metrica_criterio_t *metrica) {

	bool _buscar_load_memoria(void *elemento) {
		load_memoria_t *load = (load_memoria_t*) elemento;
		return load->id_memoria == operacion_ptr->id_memoria;
	}

	load_memoria_t *load_memoria = list_find(metrica->loads,
			&_buscar_load_memoria);
	if (load_memoria == NULL
			&& (load_memoria = construir_load_memoria(metrica,
					operacion_ptr->id_memoria)) == NULL) {
		free(operacion_ptr);
		return -1;
	}
	switch (operacion_ptr->tipo) {
	case OP_SELECT:
		metrica->reads_totales += 1;
		load_memoria->reads_realizadas += 1;
		list_add_in_index(metrica->reads, 0, operacion_ptr);
		break;
	case OP_INSERT:
		metrica->writes_totales += 1;
		load_memoria->writes_realizadas += 1;
		list_add_in_index(metrica->writes, 0, operacion_ptr);
		break;
	default:
		free(operacion_ptr);
		return -1;
	}
	return 0;
}

int registrar_operacion(tipo_operacion_t tipo, criterio_t criterio,
		uint32_t latency, uint16_t id_memoria) {

	int error;
	if ((error = pthread_mutex_lock(&mutex)) != 0) {
		return -error;
	}

	operacion_t *operacion_ptr = construir_operacion_ptr(tipo, latency,
			id_memoria);
	if (operacion_ptr == NULL) {
		return -1;
	}

	metrica_criterio_t *metrica;
	switch (criterio) {
	case CRITERIO_SC:
		metrica = &metrica_sc;
		break;
	case CRITERIO_SHC:
		metrica = &metrica_shc;
		break;
	case CRITERIO_EC:
		metrica = &metrica_ec;
		break;
	default:
		pthread_mutex_unlock(&mutex);
		return -1;
	}

	registrar_operacion_en_criterio(operacion_ptr, metrica);
	pthread_mutex_unlock(&mutex);
	return 0;
}

bool tiene_mas_de_30_segundos(void *elemento) {
	operacion_t *operacion = (operacion_t*) elemento;
	time_t timestamp_actual;
	if ((timestamp_actual = time(NULL)) == -1) {
		return 0;
	}
	return operacion->timestamp <= timestamp_actual - 30;
}

void destruir_operacion(void *elemento) {
	free(elemento);
}

void limpiar_lista_operaciones(t_list *lista) {
	operacion_t *operacion;
	while ((operacion = list_remove_by_condition(lista,
			&tiene_mas_de_30_segundos)) != NULL) {
		free(operacion);
	}
}

void limpiar_metrica(metrica_criterio_t metrica) {
	limpiar_lista_operaciones(metrica.reads);
	limpiar_lista_operaciones(metrica.writes);
}

void limpiar_metricas() {
	limpiar_metrica(metrica_sc);
	limpiar_metrica(metrica_shc);
	limpiar_metrica(metrica_ec);
}

uint32_t promedio_latency(t_list *lista) {

	uint32_t latency_total = 0;
	int tamanio_lista = list_size(lista);

	if (tamanio_lista == 0) {
		return 0;
	}

	void _sumar_latency(void *elemento) {
		operacion_t *operacion = (operacion_t*) elemento;
		latency_total += operacion->latency;
	}

	list_iterate(lista, &_sumar_latency);

	return latency_total / tamanio_lista;
}

int concat(char **destino, char *fuente, int *tamanio_actual_destino) {
	// Usamos +2 en vez de +1 para asegurarnos de que, en caso de que se
	// haya llamado a concat_con_nueva_linea, entre en \n
	int tamanio_necesario = strlen(fuente) + strlen(*destino) + 2;
	if (tamanio_necesario > *tamanio_actual_destino) {
		char* nuevo_destino;
		// Usamos tamanio_necesario por 2 para dejar lugar a futuras
		// escrituras y no hacer un nuevo realloc para cada una
		int tamanio_agrandado = tamanio_necesario * 2;
		if ((nuevo_destino = realloc(*destino, tamanio_agrandado)) == NULL) {
			free(*destino);
			*destino = NULL;
			return -1;
		}
		*destino = nuevo_destino;
		*tamanio_actual_destino = tamanio_agrandado;
	}
	strcat(*destino, fuente);
	return 0;
}

int concat_con_nueva_linea(char **destino, char *fuente,
		int *tamanio_actual_destino) {
	if (concat(destino, fuente, tamanio_actual_destino) < 0) {
		return -1;
	}
	concat(destino, "\n", tamanio_actual_destino);
	return 0;
}

int latency_string(t_list *lista, char *titulo, char **destino,
		int *tamanio_actual_destino) {
	char buffer_conversiones[15] = { 0 };
	if (concat(destino, titulo, tamanio_actual_destino) < 0) {
		return -1;
	}
	sprintf(buffer_conversiones, "%d", promedio_latency(lista));
	if (concat_con_nueva_linea(destino, buffer_conversiones,
			tamanio_actual_destino) < 0) {
		return -1;
	}
	return 0;
}

int read_latency_string(metrica_criterio_t metrica, char **destino,
		int *tamanio_actual_destino) {
	if (latency_string(metrica.reads, "Read Latency / 30s: ", destino,
			tamanio_actual_destino) < 0) {
		return -1;
	}
	return 0;
}

int write_latency_string(metrica_criterio_t metrica, char **destino,
		int *tamanio_actual_destino) {
	if (latency_string(metrica.writes, "Write Latency / 30s: ", destino,
			tamanio_actual_destino) < 0) {
		return -1;
	}
	return 0;
}

int reads_totales_string(metrica_criterio_t metrica, char **destino,
		int *tamanio_actual_destino) {
	char buffer_conversiones[15] = { 0 };
	if (concat(destino, "Reads / 30s: ", tamanio_actual_destino) < 0) {
		return -1;
	}
	sprintf(buffer_conversiones, "%d", list_size(metrica.reads));
	if (concat_con_nueva_linea(destino, buffer_conversiones,
			tamanio_actual_destino) < 0) {
		return -1;
	}
	return 0;
}

int writes_totales_string(metrica_criterio_t metrica, char **destino,
		int *tamanio_actual_destino) {
	char buffer_conversiones[15] = { 0 };
	if (concat(destino, "Writes / 30s: ", tamanio_actual_destino) < 0) {
		return -1;
	}
	sprintf(buffer_conversiones, "%d", list_size(metrica.writes));
	if (concat_con_nueva_linea(destino, buffer_conversiones,
			tamanio_actual_destino) < 0) {
		return -1;
	}
	return 0;
}

int metricas_memorias_string(metrica_criterio_t metrica, char **destino,
		int *tamanio_actual_destino) {
	if (concat_con_nueva_linea(destino, "Memory Loads", tamanio_actual_destino)
			< 0) {
		return -1;
	}

	void _concatenar_memory_load(void *elemento) {
		char id_memoria_string[10] = { 0 };
		char select_load_string[10] = { 0 };
		char insert_load_string[10] = { 0 };
		load_memoria_t *load = (load_memoria_t*) elemento;

		if (*destino == NULL) {
			return;
		}

		int id_memoria = load->id_memoria;
		sprintf(id_memoria_string, "%d", id_memoria);
		double select_load =
				metrica.reads_totales == 0 ?
						0 :
						(double) load->reads_realizadas / metrica.reads_totales;
		double insert_load =
				metrica.writes_totales == 0 ?
						0 :
						(double) load->writes_realizadas
								/ metrica.writes_totales;
		sprintf(select_load_string, "%.3f", select_load);
		sprintf(insert_load_string, "%.3f", insert_load);

		if (concat(destino, "  Memoria ", tamanio_actual_destino) < 0) {
			return;
		}
		if (concat(destino, id_memoria_string, tamanio_actual_destino) < 0) {
			return;
		}
		if (concat(destino, ":\n\tRead load: ", tamanio_actual_destino) < 0) {
			return;
		}
		if (concat(destino, select_load_string, tamanio_actual_destino) < 0) {
			return;
		}
		if (concat(destino, "\n\tWrite load: ", tamanio_actual_destino) < 0) {
			return;
		}
		if (concat_con_nueva_linea(destino, insert_load_string,
				tamanio_actual_destino) < 0) {
			return;
		}
	}

	list_iterate(metrica.loads, &_concatenar_memory_load);
	return *destino == NULL ? -1 : 0;
}

int generar_metricas_de_un_criterio(metrica_criterio_t metrica, char *titulo,
		char **destino, int *tamanio_actual_destino) {
	if (concat_con_nueva_linea(destino, titulo, tamanio_actual_destino) < 0) {
		return -1;
	}
	if (read_latency_string(metrica, destino, tamanio_actual_destino) < 0) {
		return -1;
	}
	if (write_latency_string(metrica, destino, tamanio_actual_destino) < 0) {
		return -1;
	}
	if (reads_totales_string(metrica, destino, tamanio_actual_destino) < 0) {
		return -1;
	}
	if (writes_totales_string(metrica, destino, tamanio_actual_destino) < 0) {
		return -1;
	}
	if (metricas_memorias_string(metrica, destino, tamanio_actual_destino)
			< 0) {
		return -1;
	}
	return 0;
}

char *obtener_metricas() {
	int error;
	if ((error = pthread_mutex_lock(&mutex)) != 0) {
		return NULL;
	}
	limpiar_metricas();
	int tamanio_actual = 300;
	char *string_metricas = calloc(tamanio_actual, sizeof(char));
	metrica_criterio_t metricas[3] = { metrica_sc, metrica_shc, metrica_ec };
	char *titulos[3] = { "\n\tMetrica SC\n", "\n\tMetrica SHC\n",
			"\n\tMetrica EC\n" };
	if (string_metricas == NULL) {
		return NULL;
	}
	for (int i = 0; i < 3; i++) {
		if (generar_metricas_de_un_criterio(metricas[i], titulos[i],
				&string_metricas, &tamanio_actual) < 0) {
			return NULL;
		}
	}
	pthread_mutex_unlock(&mutex);
	return string_metricas;
}
