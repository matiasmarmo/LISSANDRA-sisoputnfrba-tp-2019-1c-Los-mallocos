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
#include "memoria-requests-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

//pthread_mutex_t memoria_main_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t memoria_main_cond = PTHREAD_COND_INITIALIZER;


void inicializar_memoria() {

}

void liberar_recursos_memoria() {

}


void finalizar_memoria(){

}
uint8_t* memoria;
int tamanio_maximo_value;

uint8_t* get_memoria(){
	return memoria;
}

int get_tamanio_maximo_pagina(){
	return sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES
}

int main() {
	//inicializar_memoria();
	int tamanio_memoria = 45; //int tamanio_memoria = get_tamanio_memoria();
	tamanio_maximo_value = 10; // PEDIRSELO AL FS (BYTES)
	int tamanio_maximo_pagina = sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES
	memoria = calloc(tamanio_memoria, sizeof(char));
	inicializacion_tabla_segmentos();
	// creo 2 segmentos para probar
	crear_segmento_nuevo("tabla1");
	crear_segmento_nuevo("tabla2");
	// pruebo si hay o no segmento en memoria
	segmento* segmento_buscado = encontrar_segmento_en_memoria("tabla1");
	if(segmento_buscado==NULL){
		printf("segmento no esta en memoria\n");
	}
	else{
		printf("segmento_buscado -> reg_lim: %d\n",segmento_buscado->registro_limite);
		printf("segmento_buscado -> nombre: %s\n",segmento_buscado->tabla);
	}
	cantidad_paginas_de_un_segmento(segmento_buscado);
	int lugar_pagina_vacia;
	//--------------------------------------------------------------
	lugar_pagina_vacia = encontrar_pagina_vacia(memoria,tamanio_memoria,tamanio_maximo_pagina);
	if(lugar_pagina_vacia == -1){
		printf("no hay mas paginas libres");
	}else{
		crear_registro_nuevo_en_tabla_de_paginas(memoria, lugar_pagina_vacia, segmento_buscado, 0);
		crear_pagina_nueva(memoria,lugar_pagina_vacia, 25, 123456, "martin");
	}
	cantidad_paginas_de_un_segmento(segmento_buscado);
	//--------------------------------------------------------------
	lugar_pagina_vacia = encontrar_pagina_vacia(memoria,tamanio_memoria,tamanio_maximo_pagina);
	if(lugar_pagina_vacia == -1){
		printf("no hay mas paginas libres");
	}else{
		crear_registro_nuevo_en_tabla_de_paginas(memoria, lugar_pagina_vacia, segmento_buscado, 0);
		crear_pagina_nueva(memoria,lugar_pagina_vacia, 12, 8976, "carlos");
	}
	cantidad_paginas_de_un_segmento(segmento_buscado);
	//--------------------------------------------------------------

	registro_tabla_pagina* a = list_get(segmento_buscado->registro_base,0);

	printf("numero de pagina: %d\n", a->numero_pagina);
	printf("puntero a pagina: %p\n", a->puntero_a_pagina);
	printf("flag  modificado: %d\n", a->flag_modificado);
	printf("puntero a pagina: %p\n", &memoria[0]);
	pagina final1;
	pagina final2;
	final1.key = (uint16_t*)(memoria + 0);
	final2.key = (uint16_t*)(memoria + 21);
	printf("key1: %d\n", *(final1.key));
	printf("key2: %d\n", *(final2.key));
	cantidad_paginas_de_un_segmento(segmento_buscado);

	printf("Busco la pagina con key = 25\n");
	registro_tabla_pagina* reg_pagina = encontrar_pagina_en_memoria(segmento_buscado, 25);
	if(reg_pagina==NULL){
		printf("   La pagina con esa key no esta en memoria\n");
	}
	else{
		printf("   Pagina buscada -> key: %d\n",*((uint16_t*)(reg_pagina->puntero_a_pagina)));
		printf("   Pagina buscada -> timestamp: %llu\n",*((uint64_t*)(reg_pagina->puntero_a_pagina + 2)));
		int h=0;
		while(*((char*)(reg_pagina->puntero_a_pagina + 10 + h)) != '\0'){
			printf("   Pagina buscada -> value: %c\n",*((char*)(reg_pagina->puntero_a_pagina + 10 + h)));
			h++;
		}

	}

	printf("lugar_pagina_vacia: %d",lugar_pagina_vacia);

	free(memoria);

	//int paginas = tamanio_memoria / tamanio_maximo_pagina;

	/*inicializar_memoria_logger();
	inicializar_memoria_config();
	lissandra_thread_t l_thread;
	l_thread_create(&l_thread, &correr_servidor_memoria, NULL);
	sleep(1000000);
	l_thread_solicitar_finalizacion(&l_thread);
	l_thread_join(&l_thread, NULL);
	destruir_memoria_logger();
	destruir_memoria_config();*/

	// inicializo hilos

	//pthread_mutex_lock(&memoria_main_mutex);
	//pthread_cond_wait(&memoria_main_cond, &memoria_main_mutex);
	//pthread_mutex_unlock(&memoria_main_mutex);

	// finalizar hilos

	//liberar_recursos_memoria();
	return 0;
}




