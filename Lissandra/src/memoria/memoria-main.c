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
#include "memoria-gossip.h"
#include "memoria-request-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"
#include "memoria-handler.h"
#include "conexion-lfs.h"

pthread_mutex_t memoria_main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t memoria_main_cond = PTHREAD_COND_INITIALIZER;

int tamanio_maximo_value = -1;

void set_tamanio_value(int tamanio) {
	tamanio_maximo_value = tamanio;
}

void inicializar_memoria(){
	// Verificar si alguno falla. Si lo hace alguno, 
	// terminar la memoria
	// La terminamos con exit(EXIT_FAILURE);
	inicializar_memoria_config();
	inicializar_memoria_logger();
	inicializacion_memoria();
	inicializacion_tabla_segmentos();
	inicializacion_tabla_gossip();
	conectar_lfs();
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

int main() {

	inicializar_memoria();

	lissandra_thread_t l_thread;
	lissandra_thread_periodic_t thread_gossip;
	l_thread_create(&l_thread, &correr_servidor_memoria, NULL);
	l_thread_periodic_create(&thread_gossip, &realizar_gossip_threaded, &get_tiempo_gossiping, NULL);

	pthread_mutex_lock(&memoria_main_mutex);
	pthread_cond_wait(&memoria_main_cond, &memoria_main_mutex);
	pthread_mutex_unlock(&memoria_main_mutex);

	l_thread_solicitar_finalizacion(&l_thread);
	l_thread_solicitar_finalizacion(&thread_gossip.l_thread);
	l_thread_join(&l_thread, NULL);
	l_thread_join(&thread_gossip.l_thread, NULL);

	liberar_recursos_memoria();
	return 0;
}
