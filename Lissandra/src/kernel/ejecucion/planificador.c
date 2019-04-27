#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <commons/log.h>

#include "../../commons/lissandra-threads.h"
#include "../kernel-config.h"
#include "../kernel-logger.h"
#include "exec.h"
#include "planificador.h"

pthread_mutex_t cola_new_mutex = PTHREAD_MUTEX_INITIALIZER;
t_queue *cola_new;
t_queue *cola_ready;

void destruir_nuevo_script(nuevo_script_t *nuevo_script) {
	free(nuevo_script->str);
	free(nuevo_script);
}

void destruir_scb(SCB *scb) {
	free(scb->source);
	free(scb);
}

void script_new_a_finalizado(nuevo_script_t *nuevo_script) {
	destruir_nuevo_script(nuevo_script);
}

void script_exec_a_finalizado(SCB *scb) {
	destruir_scb(scb);
}

int inicializar_planificador(lissandra_thread_t **threads, int cantidad_threads) {
	for (int i = 0; i < cantidad_threads; i++) {
		threads[i] = NULL;
	}
	cola_new = queue_create();
	cola_ready = queue_create();
	return 0;
}

void destruir_planificador() {

	void _destruir_scb(void *elemento) {
		destruir_scb((SCB*) elemento);
	}

	void _destruir_nuevo_script(void *elemento) {
		destruir_nuevo_script((nuevo_script_t*) elemento);
	}

	queue_destroy_and_destroy_elements(cola_new, &_destruir_nuevo_script);
	queue_destroy_and_destroy_elements(cola_ready, &_destruir_scb);
}

int agregar_nuevo_script(bool es_request_unitario, char *str) {
	nuevo_script_t *nuevo_script = malloc(sizeof(nuevo_script_t));
	if (nuevo_script == NULL) {
		return -1;
	}
	nuevo_script->es_request_unitario = es_request_unitario;
	nuevo_script->str= strdup(str);
	if (nuevo_script->str == NULL) {
		free(nuevo_script);
		return -1;
	}
	if (pthread_mutex_lock(&cola_new_mutex) != 0) {
		destruir_nuevo_script(nuevo_script);
		return -1;
	}
	if(cola_new == NULL) {
		destruir_nuevo_script(nuevo_script);
		pthread_mutex_unlock(&cola_new_mutex);
		return -1;
	}
	queue_push(cola_new, nuevo_script);
	pthread_mutex_unlock(&cola_new_mutex);
	return 0;
}

void despachar_script_detenido(SCB *script) {
	switch (script->estado) {
	case INT_FIN_QUANTUM:
	case ERROR_MISC:
		queue_push(cola_ready, script);
		break;
	case SCRIPT_FINALIZADO:
	case ERROR_SCRIPT:
		script_exec_a_finalizado(script);
		break;
	default:
		break;
	}
}

int alojar_siguiente_script(lissandra_thread_t **runner_thread) {
	SCB *primer_script_ready = queue_pop(cola_ready);
	if (primer_script_ready == NULL) {
		return -1;
	}
	if((*runner_thread = malloc(sizeof(lissandra_thread_t))) == NULL) {
		return -1;
	}
	l_thread_create(*runner_thread, &ejecutar_script, primer_script_ready);
	return 0;
}

char *leer_fuente_script(char *path) {
	FILE *archivo = fopen(path, "r");
	if (archivo == NULL) {
		return NULL;
	}
	fseek(archivo, 0, SEEK_END);
	long tamanio_archivo = ftell(archivo);
	rewind(archivo);
	char *source = calloc(tamanio_archivo + 1, sizeof(char));
	if (source == NULL) {
		fclose(archivo);
		return NULL;
	}
	if (fread(source, tamanio_archivo, 1, archivo) != 1) {
		fclose(archivo);
		free(source);
		return NULL;
	}
	fclose(archivo);
	return source;
}

SCB *construir_scb(nuevo_script_t *nuevo_script) {
	SCB *scb = malloc(sizeof(SCB));
	if (scb == NULL) {
		return NULL;
	}
	scb->es_request_unitario = nuevo_script->es_request_unitario;
	if(nuevo_script->es_request_unitario) {
		scb->source = strdup(nuevo_script->str);
	} else {
		scb->source = leer_fuente_script(nuevo_script->str);
	}
	if (scb->source == NULL) {
		free(scb);
		return NULL;
	}
	scb->request_pointer = 0;
	scb->estado = NUEVO;
	return scb;
}

void admitir_nuevos_scripts() {
	nuevo_script_t *nuevo_script;
	if (pthread_mutex_lock(&cola_new_mutex) != 0) {
		return;
	}
	while ((nuevo_script = queue_pop(cola_new)) != NULL) {
		SCB *nuevo_scb = construir_scb(nuevo_script);
		if (nuevo_scb == NULL) {
			kernel_log_to_level(LOG_LEVEL_ERROR, false, "Error al admitir nuevo script.");
			script_new_a_finalizado(nuevo_script);
			continue;
		}
		queue_push(cola_ready, nuevo_scb);
		destruir_nuevo_script(nuevo_script);
	}
	pthread_mutex_unlock(&cola_new_mutex);
}

void replanificar(lissandra_thread_t **runner_thread) {
	if(*runner_thread != NULL) {
		l_thread_join(*runner_thread, NULL);
		despachar_script_detenido((SCB*) (*runner_thread)->entrada);
		l_thread_destroy(*runner_thread);
		free(*runner_thread);
		*runner_thread = NULL;
	}
	if (alojar_siguiente_script(runner_thread) == 0) {
		return;
	}
	admitir_nuevos_scripts();
	alojar_siguiente_script(runner_thread);
}

void *correr_planificador(void *entrada) {
	lissandra_thread_t *self = (lissandra_thread_t*) entrada;
	int cantidad_runner_threads = get_multiprocesamiento();
	lissandra_thread_t *runner_threads[cantidad_runner_threads];
	inicializar_planificador(runner_threads, cantidad_runner_threads);
	kernel_log_to_level(LOG_LEVEL_INFO, false, "Esperando scripts.");
	while (!l_thread_debe_finalizar(self)) {
		for (int i = 0; i < cantidad_runner_threads; i++) {
			if (runner_threads[i] == NULL || l_thread_finalizo(runner_threads[i])) {
				replanificar(&runner_threads[i]);
			}
		}
		usleep(200000);
	}
	kernel_log_to_level(LOG_LEVEL_INFO, false, "Finalizando planificador.");
	for(int i = 0; i < cantidad_runner_threads; i++) {
		if(runner_threads[i] != NULL) {
			l_thread_solicitar_finalizacion(runner_threads[i]);
			l_thread_join(runner_threads[i], NULL);
			l_thread_destroy(runner_threads[i]);
			if((runner_threads[i])->entrada != NULL) {
				destruir_scb((SCB*)(runner_threads[i])->entrada);
			}
			free(runner_threads[i]);
		}
	}
	destruir_planificador();
	pthread_exit(NULL);
}
