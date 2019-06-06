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

int get_tamanio_maximo_pagina() {
	return sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES
}
void finalizar_memoria() {
	pthread_mutex_lock(&memoria_main_mutex);
	pthread_cond_signal(&memoria_main_cond);
	pthread_mutex_unlock(&memoria_main_mutex);
}

int main() {
	//inicializar_memoria();
	inicializar_memoria_config();
	inicializar_clientes();
	tamanio_maximo_value = 10; // PEDIRSELO AL FS (BYTES)
	inicializacion_memoria();
	inicializacion_tabla_segmentos();

	/*struct select_request query;
	 struct insert_request query2;
	 init_insert_request("tabla1", 25, "martin",123456, &query2);
	 printf("INSERT tabla1 25 martin\n");
	 _insert(query2);
	 destroy_insert_request(&query2);
	 init_insert_request("tabla1", 12, "carlos",8796, &query2);
	 printf("INSERT tabla1 12 carlos\n");
	 _insert(query2);
	 destroy_insert_request(&query2);
	 init_insert_request("tabla1", 73, "matias",1961, &query2);
	 printf("INSERT tabla1 73 matias\n");
	 _insert(query2);
	 destroy_insert_request(&query2);

	 estado_actual_memoria();
	 //--------------------------
	 init_select_request("tabla1", 12, &query);
	 printf("SELECT tabla1 12\n");
	 _select(query);
	 destroy_select_request(&query);
	 //--------------------------
	 init_select_request("tabla1", 72, &query);
	 printf("SELECT tabla1 72\n");
	 _select(query);
	 destroy_select_request(&query);
	 //--------------------------
	 init_select_request("tabla3", 72, &query);
	 printf("SELECT tabla3 72\n");
	 _select(query);
	 destroy_select_request(&query);
	 //--------------------------
	 init_select_request("tabla1", 73, &query);
	 printf("SELECT tabla1 73\n");
	 _select(query);
	 destroy_select_request(&query);
	 //--------------------------
	 estado_actual_memoria();
	 init_insert_request("tabla2", 66, "alfajor",1234, &query2);
	 printf("INSERT tabla2 66 alfajor\n");
	 _insert(query2);
	 destroy_insert_request(&query2);
	 //--------------------------
	 init_select_request("tabla2", 66, &query);
	 printf("SELECT tabla2 66\n");
	 _select(query);
	 destroy_select_request(&query);
	 */
	//--------------------------
	//--------------------------
	//printf(".:SUCCESS:.\n\n");
	//int paginas = tamanio_memoria / tamanio_maximo_pagina;

	inicializar_memoria_logger();
	inicializar_memoria_config();
	lissandra_thread_t l_thread;
	l_thread_create(&l_thread, &correr_servidor_memoria, NULL);

	pthread_mutex_lock(&memoria_main_mutex);
	pthread_cond_wait(&memoria_main_cond, &memoria_main_mutex);
	pthread_mutex_unlock(&memoria_main_mutex);

	l_thread_solicitar_finalizacion(&l_thread);
	l_thread_join(&l_thread, NULL);
	estado_actual_memoria();
	destruccion_tabla_registros_paginas();
	destruccion_tabla_segmentos();
	destruccion_memoria();
	destruir_memoria_config();
	destruir_memoria_logger();
	return 0;
}