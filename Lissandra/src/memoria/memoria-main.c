#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "../commons/lissandra-threads.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

struct segmento{
	char* tabla;
	void* registro_base;
	void* registro_limite;
};
struct registro_tabla_pagina{
	uint16_t numero_pagina;
	void* puntero_a_pagina; //MARCO
	uint8_t flag_modificado;
};
struct pagina{
	uint64_t timestamp;
	uint16_t key;
	char* value;
};


//pthread_mutex_t memoria_main_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t memoria_main_cond = PTHREAD_COND_INITIALIZER;


void inicializar_memoria() {

}
/*
void liberar_recursos_memoria() {

}
*/

void finalizar_memoria(){

}

int main() {
	//inicializar_memoria();
	int tamanio_memoria = get_tamanio_memoria();
	//char* memoria = calloc(tamanio_memoria, sizeof(char));

	t_list* TABLA_DE_SEGMENTOS = list_create();
	t_list* TABLA_DE_PAGINAS = list_create();

	inicializar_memoria_logger();
	inicializar_memoria_config();
	lissandra_thread_t l_thread;
	l_thread_create(&l_thread, &correr_servidor_memoria, NULL);
	sleep(1000000);
	l_thread_solicitar_finalizacion(&l_thread);
	l_thread_join(&l_thread, NULL);
	destruir_memoria_logger();
	destruir_memoria_config();
	return 0;

	// inicializo hilos

	//pthread_mutex_lock(&memoria_main_mutex);
	//pthread_cond_wait(&memoria_main_cond, &memoria_main_mutex);
	//pthread_mutex_unlock(&memoria_main_mutex);

	// finalizar hilos

	//liberar_recursos_memoria();
	return 0;
}
