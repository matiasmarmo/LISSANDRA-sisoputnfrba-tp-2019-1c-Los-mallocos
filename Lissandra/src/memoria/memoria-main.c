#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>

#include "../commons/lissandra-threads.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

#define MAX_TAM_NOMBRE_TABLA 50

typedef struct segmento_t{
	char tabla[MAX_TAM_NOMBRE_TABLA];
	t_list* registro_base;
	int registro_limite;
}segmento;

typedef struct registro_tabla_pagina_t{
	uint16_t numero_pagina;
	uint8_t* puntero_a_pagina; //MARCO
	uint8_t flag_modificado;
}registro_tabla_pagina;

typedef struct pagina_t{
	uint64_t* timestamp;
	uint16_t* key;
	char* value;
}pagina;


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

void crear_registro_nuevo_en_tabla_de_paginas(uint8_t* memoria, int lugar_pagina_vacia, segmento* segmento, int flag_modificado){
	registro_tabla_pagina* nuevo_registro_pagina = malloc(sizeof(registro_tabla_pagina));
	nuevo_registro_pagina->numero_pagina = list_size(segmento->registro_base) + 1;
	nuevo_registro_pagina->puntero_a_pagina = memoria + lugar_pagina_vacia;
	nuevo_registro_pagina->flag_modificado = flag_modificado;
	list_add(segmento->registro_base, nuevo_registro_pagina);
}

void crear_pagina_nueva(uint8_t* memoria,int lugar_pagina_vacia, uint16_t key, uint64_t timestamp, char* value){
	pagina nueva_pagina;
	nueva_pagina.key = (uint16_t*)(memoria + 0 + lugar_pagina_vacia);
	nueva_pagina.timestamp = (uint64_t*)(memoria + sizeof(uint16_t) + lugar_pagina_vacia);
	nueva_pagina.value = (char*)(memoria + sizeof(uint16_t) + sizeof(uint64_t) + lugar_pagina_vacia);
	*(nueva_pagina.key) = key;
	*(nueva_pagina.timestamp) = timestamp;
	strcpy(nueva_pagina.value,value);
}

void crear_segmento_nuevo(t_list* tabla_segmentos,char* nombre_tabla_nueva){
	segmento* nuevo_segmento = malloc(sizeof(segmento));
	strcpy(nuevo_segmento->tabla , nombre_tabla_nueva);
	nuevo_segmento->registro_base = list_create();
	nuevo_segmento->registro_limite = 0;
	list_add(tabla_segmentos, nuevo_segmento);
}

registro_tabla_pagina* encontrar_pagina_en_memoria(segmento* segmento, uint16_t key){
	bool _buscar_pagina_en_memoria(void *elemento) {
		registro_tabla_pagina *reg = (registro_tabla_pagina*) elemento;
		pagina final;
		final.key = (uint16_t*)(reg->puntero_a_pagina);
		return *(final.key) == key;
	}
	registro_tabla_pagina* reg_pagina = list_find(segmento->registro_base, &_buscar_pagina_en_memoria);
	return reg_pagina;
}

segmento* encontrar_segmento_en_memoria(t_list* tabla_segmentos, char* nombre_tabla_buscada){
	bool _buscar_segmento_en_memoria(void *elemento) {
		segmento *b = (segmento*) elemento;
		return !strcmp(b->tabla , nombre_tabla_buscada);
	}
	segmento *segmento = list_find(tabla_segmentos, &_buscar_segmento_en_memoria);
	return segmento;
}

int encontrar_pagina_vacia(uint8_t* memoria,int tamanio_memoria,int tamanio_maximo_pagina){
	int numero_pagina = 0;
	while(tamanio_memoria >= tamanio_maximo_pagina + numero_pagina){
		if(	*(memoria + numero_pagina) == 0){
			return numero_pagina;
		}
		numero_pagina += tamanio_maximo_pagina;
	}
	return -1;
}
int cantidad_paginas_de_un_segmento(segmento* segmento){
	int cantidad = list_size(segmento->registro_base);
	printf("cant_paginas: %d\n", cantidad);
	return cantidad;
}

int main() {
	//inicializar_memoria();
	int tamanio_memoria = 45; //int tamanio_memoria = get_tamanio_memoria();
	int tamanio_maximo_value = 10; // PEDIRSELO AL FS (BYTES)
	int tamanio_maximo_pagina = sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES
	uint8_t* memoria = calloc(tamanio_memoria, sizeof(char));
	t_list* TABLA_DE_SEGMENTOS = list_create();
	// creo 2 segmentos para probar
	crear_segmento_nuevo(TABLA_DE_SEGMENTOS,"tabla1");
	crear_segmento_nuevo(TABLA_DE_SEGMENTOS,"tabla2");
	// pruebo si hay o no segmento en memoria
	segmento* segmento_buscado = encontrar_segmento_en_memoria(TABLA_DE_SEGMENTOS, "tabla1");
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
		printf("   Pagina buscada -> timestamp: %d\n",*((uint64_t*)(reg_pagina->puntero_a_pagina + 2)));
		int h=0;
		while(*((char*)(reg_pagina->puntero_a_pagina + 10 + h)) != '\0'){
			printf("   Pagina buscada -> value: %c\n",*((char*)(reg_pagina->puntero_a_pagina + 10 + h)));
			h++;
		}

	}

	printf("lugar_pagina_vacia: %d",lugar_pagina_vacia);

	free(memoria);
	list_destroy(TABLA_DE_SEGMENTOS);

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




