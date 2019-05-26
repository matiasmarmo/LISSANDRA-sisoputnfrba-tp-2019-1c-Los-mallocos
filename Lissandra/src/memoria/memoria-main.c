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
	nueva_pagina.timestamp = (uint64_t*)(memoria + 2 + lugar_pagina_vacia);
	nueva_pagina.value = (char*)(memoria + 10 + lugar_pagina_vacia);
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
	// le agrego a ese segmento 1 pagina
	cantidad_paginas_de_un_segmento(segmento_buscado);

	int lugar_pagina_vacia;

	/*registro_tabla_pagina* nuevo_registro_pagina = malloc(sizeof(registro_tabla_pagina));
	nuevo_registro_pagina->numero_pagina = list_size(segmento_buscado->registro_base) + 1;
	nuevo_registro_pagina->puntero_a_pagina = memoria + lugar_pagina_vacia;
	nuevo_registro_pagina->flag_modificado = 0;
	list_add(segmento_buscado->registro_base, nuevo_registro_pagina);*/
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
	/*registro_tabla_pagina* nuevo_registro_pagina3 = malloc(sizeof(registro_tabla_pagina));
		nuevo_registro_pagina3->numero_pagina = list_size(segmento_buscado->registro_base) + 1;
		nuevo_registro_pagina3->puntero_a_pagina = memoria + lugar_pagina_vacia;
		nuevo_registro_pagina3->flag_modificado = 0;
		list_add(segmento_buscado->registro_base, nuevo_registro_pagina3);*/
	//cantidad_paginas_de_un_segmento(segmento_buscado);
	/*if(lugar_pagina_vacia == -1){
		printf("no hay mas paginas libres");
	}else{
		crear_pagina_nueva(memoria,lugar_pagina_vacia, 12, 8976, "carlos");
	}*/

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
/*
	pagina nueva_pagina2;
	nueva_pagina2.key = (uint16_t*)(memoria + 21);
	nueva_pagina2.timestamp = (uint64_t*)(memoria + 23);
	nueva_pagina2.value = (char*)(memoria + 31);
	*(nueva_pagina2.key)=3;
	*(nueva_pagina2.timestamp) = 1;
	strcpy(nueva_pagina2.value,"matias");*/

	//int tamanio_maximo_pagina = sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES

	//int lugar_pagina_vacia = encontrar_pagina_vacia(memoria,tamanio_memoria,tamanio_maximo_pagina);
	printf("lugar_pagina_vacia: %d",lugar_pagina_vacia);

	free(memoria);
	list_destroy(TABLA_DE_SEGMENTOS);
	//int paginas = tamanio_memoria / tamanio_maximo_pagina;
	// t_list* TABLA_DE_SEGMENTOS = list_create();
	//	int tamanio_memoria = 1000;//get_tamanio_memoria();
		//struct pagina pagina;
	//	t_list* TABLA_DE_SEGMENTOS = list_create();
		//t_list* PAGINAS = list_create();
	//	struct segmento a;
	//	a.tabla = "tabla1";
	//	a.registro_base = NULL;
	//	a.registro_limite = NULL;
/*
		int j;
		j=list_size(TABLA_DE_SEGMENTOS);
		printf("inicial %d",j);
		//crear_segmento_nuevo(TABLA_DE_SEGMENTOS,"tabla1");
		//printf("%d     ",nuevo.registro_limite);
		//list_add(TABLA_DE_SEGMENTOS, &a);
		j=list_size(TABLA_DE_SEGMENTOS);
		printf("final %d",j);*/
	//char* memoria = calloc(tamanio_memoria, sizeof(char));

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

/*void crear_todas_las_paginas_del_segmento_vacias(t_list* tabla_paginas,int cantidad_paginas){
	struct registro_tabla_pagina nuevo_registro_pagina;
	nuevo_registro_pagina.numero_pagina=0;
	nuevo_registro_pagina.puntero_a_pagina=NULL;
	nuevo_registro_pagina.flag_modificado=0;
	for(int i=0;i<cantidad_paginas;i++){
		nuevo_registro_pagina.numero_pagina=i;
		list_add(tabla_paginas, &nuevo_registro_pagina);
	}
}*/




