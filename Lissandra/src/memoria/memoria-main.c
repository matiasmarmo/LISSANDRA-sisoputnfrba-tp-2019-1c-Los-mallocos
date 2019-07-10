#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>

#include "../commons/comunicacion/protocol.h"
#include "../commons/lissandra-threads.h"
#include "memoria-principal/memoria-handler.h"
#include "requests/memoria-other-requests-handler.h"
#include "memoria-gossip.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"
#include "conexion-lfs.h"

pthread_mutex_t memoria_main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t memoria_main_cond = PTHREAD_COND_INITIALIZER;

int tamanio_maximo_value = -1;

void set_tamanio_value(int tamanio) {
	tamanio_maximo_value = tamanio;
}

int get_tamanio_value() {
	return tamanio_maximo_value;
}

void inicializar_memoria(){
	if(inicializar_memoria_logger() < 0) {
		destruir_memoria_config();
		exit(EXIT_FAILURE);
	}
	if(inicializar_memoria_config() < 0) {
		memoria_log_to_level(LOG_LEVEL_ERROR, false,
			"Error al inicializar archivo de configuración de la memoria. Abortando.");
		exit(EXIT_FAILURE);
	}
	if(inicializacion_memoria() < 0) {
		memoria_log_to_level(LOG_LEVEL_ERROR, false,
			"Error al inicializar memoria en calloc. Abortando.");
		destruir_memoria_config();
		destruir_memoria_logger();
		exit(EXIT_FAILURE);
	}

	inicializacion_tabla_segmentos(); // solamente creo una lista

	if(inicializacion_tabla_gossip() < 0) {
		memoria_log_to_level(LOG_LEVEL_ERROR, false,
			"Error al inicializar tabla de gossip. Abortando.");
		destruir_memoria_config();
		destruir_memoria_logger();
		destruccion_memoria();
		destruccion_tabla_segmentos();
		exit(EXIT_FAILURE);
	}
	if(conectar_lfs() < 0) {
		memoria_log_to_level(LOG_LEVEL_ERROR, false,
			"Error al conectar memoria con lfs. Abortando.");
		destruir_memoria_config();
		destruir_memoria_logger();
		destruccion_memoria();
		destruccion_tabla_gossip();
		destruccion_tabla_segmentos();
		exit(EXIT_FAILURE);
	}
}

void liberar_recursos_memoria() {
	destruccion_tabla_gossip();
	destruccion_tabla_registros_paginas();
	destruccion_tabla_segmentos();
	destruccion_memoria();
	destruir_memoria_logger();
	destruir_memoria_config();
	desconectar_lfs();
}

int get_tamanio_maximo_pagina() {
	return sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES
}

void finalizar_memoria() {
	pthread_mutex_lock(&memoria_main_mutex);
	pthread_cond_signal(&memoria_main_cond);
	pthread_mutex_unlock(&memoria_main_mutex);
}

void finalizar_thread(lissandra_thread_t *l_thread) {
	l_thread_solicitar_finalizacion(l_thread);
	l_thread_join(l_thread, NULL);
}

void finalizar_thread_si_se_creo(lissandra_thread_t *l_thread, int create_ret) {
	// Si el create al thread retornó 0 (fue exitoso), la finalizamos.
	if (create_ret == 0) {
		finalizar_thread(l_thread);
	}
}

int main() {
	int rets[4];
	inicializar_memoria();

	lissandra_thread_t servidor, inotify;
	lissandra_thread_periodic_t thread_gossip, thread_journal;
	rets[0] = l_thread_create(&servidor, &correr_servidor_memoria, NULL);
	rets[1] = inicializar_memoria_inotify(&inotify);
	rets[2] = l_thread_periodic_create(&thread_gossip, &realizar_gossip_threaded, &get_tiempo_gossiping, NULL);
	rets[3] = l_thread_periodic_create(&thread_journal, &realizar_journal_threaded, &get_tiempo_journal, NULL);

	for(int i = 0; i < 4; i++) {
		if(rets[i] != 0) {
			finalizar_thread_si_se_creo(&servidor, rets[0]);
			finalizar_thread_si_se_creo(&inotify, rets[1]);
			finalizar_thread_si_se_creo(&thread_gossip.l_thread, rets[2]);
			finalizar_thread_si_se_creo(&thread_journal.l_thread, rets[3]);
			exit(EXIT_FAILURE);
		}
	}

	pthread_mutex_lock(&memoria_main_mutex);
	pthread_cond_wait(&memoria_main_cond, &memoria_main_mutex);
	pthread_mutex_unlock(&memoria_main_mutex);

	finalizar_thread(&servidor);
	finalizar_thread(&inotify);
	finalizar_thread(&thread_gossip.l_thread);
	finalizar_thread(&thread_journal.l_thread);

	realizar_journal();
	liberar_recursos_memoria();
	return 0;
}
