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
#include "memoria-request-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"
#include "memoria-handler.h"

pthread_mutex_t memoria_main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t memoria_main_cond = PTHREAD_COND_INITIALIZER;

int tamanio_maximo_value;

void inicializar_memoria(){
	inicializar_memoria_config();
	inicializar_memoria_logger();
	inicializar_clientes();
	inicializacion_memoria();
	inicializacion_tabla_segmentos();
}

int get_tamanio_maximo_pagina() {
	return sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES
}

void finalizar_memoria() {
	pthread_mutex_lock(&memoria_main_mutex);
	pthread_cond_signal(&memoria_main_cond);
	pthread_mutex_unlock(&memoria_main_mutex);
}

int main() {
	inicializar_memoria();

	tamanio_maximo_value = 15; // PEDIRSELO AL FS (BYTES) HANDSHAKE

	lissandra_thread_t l_thread;
	l_thread_create(&l_thread, &correr_servidor_memoria, NULL);

	pthread_mutex_lock(&memoria_main_mutex);
	pthread_cond_wait(&memoria_main_cond, &memoria_main_mutex);
	pthread_mutex_unlock(&memoria_main_mutex);

	l_thread_solicitar_finalizacion(&l_thread);
	l_thread_join(&l_thread, NULL);
	destruccion_tabla_registros_paginas();
	destruccion_tabla_segmentos();
	destruccion_memoria();
	destruir_memoria_logger();
	destruir_memoria_config();
	return 0;
}
