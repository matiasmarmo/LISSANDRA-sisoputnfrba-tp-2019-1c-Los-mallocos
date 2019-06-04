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
#include "memoria-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

#define MAX_TAM_NOMBRE_TABLA 50

t_list* TABLA_DE_SEGMENTOS;
uint8_t* memoria;

uint8_t* get_memoria(){
	return memoria;
}
void inicializacion_memoria(int tamanio_memoria){
	memoria = calloc(tamanio_memoria, sizeof(char));
}
void destruccion_memoria(){
	free(memoria);
}

void inicializacion_tabla_segmentos(){
	TABLA_DE_SEGMENTOS = list_create();
}

void destruccion_tabla_registros_paginas(){
	void _destroy_element(void *elemento) {
		free((registro_tabla_pagina*) elemento);
	}
	int cantidad_segmentos = list_size(TABLA_DE_SEGMENTOS);
	segmento* segmento_temporal;
	for(int i=0; i< cantidad_segmentos; i++){
		segmento_temporal = list_get(TABLA_DE_SEGMENTOS,i);
		list_destroy_and_destroy_elements(segmento_temporal->registro_base,&_destroy_element);;
	}
	free(segmento_temporal);
}

void destruccion_tabla_segmentos(){
	void _destroy_element(void *elemento) {
		free((segmento*) elemento);
	}
	list_destroy_and_destroy_elements(TABLA_DE_SEGMENTOS,&_destroy_element);;
}

uint16_t numero_de_pagina_libre(segmento* segmento){
	uint16_t cantidad_paginas = list_size(segmento->registro_base);
	uint16_t num_pagina = 1;
	bool _buscar_numero_pagina(void *elemento) {
		registro_tabla_pagina *reg = (registro_tabla_pagina*) elemento;
		return reg->numero_pagina == num_pagina;
	}
	for(num_pagina = 1 ; num_pagina < cantidad_paginas ; num_pagina++){
		registro_tabla_pagina* reg_pagina = list_find(segmento->registro_base, &_buscar_numero_pagina);
		if(reg_pagina == NULL){
			return num_pagina;
		}
	}
	return cantidad_paginas;
}

void crear_registro_nuevo_en_tabla_de_paginas(int lugar_pagina_vacia, segmento* segmento, int flag_modificado){
	registro_tabla_pagina* nuevo_registro_pagina = malloc(sizeof(registro_tabla_pagina));
	nuevo_registro_pagina->numero_pagina = numero_de_pagina_libre(segmento);
	//nuevo_registro_pagina->numero_pagina = list_size(segmento->registro_base) + 1;
	nuevo_registro_pagina->puntero_a_pagina = memoria + lugar_pagina_vacia;
	nuevo_registro_pagina->flag_modificado = flag_modificado;
	list_add(segmento->registro_base, nuevo_registro_pagina);
}

void crear_pagina_nueva(int lugar_pagina_vacia, uint16_t key, uint64_t timestamp, char* value){
	pagina nueva_pagina;
	nueva_pagina.key = (uint16_t*)(memoria + 0 + lugar_pagina_vacia);
	nueva_pagina.timestamp = (uint64_t*)(memoria + sizeof(uint16_t) + lugar_pagina_vacia);
	nueva_pagina.value = (char*)(memoria + sizeof(uint16_t) + sizeof(uint64_t) + lugar_pagina_vacia);
	*(nueva_pagina.key) = key;
	*(nueva_pagina.timestamp) = timestamp;
	strcpy(nueva_pagina.value,value);
}

void crear_segmento_nuevo(char* nombre_tabla_nueva){
	segmento* nuevo_segmento = malloc(sizeof(segmento));
	strcpy(nuevo_segmento->tabla , nombre_tabla_nueva);
	nuevo_segmento->registro_base = list_create();
	nuevo_segmento->registro_limite = 0;
	list_add(TABLA_DE_SEGMENTOS, nuevo_segmento);
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

segmento* encontrar_segmento_en_memoria(char* nombre_tabla_buscada){
	bool _buscar_segmento_en_memoria(void *elemento) {
		segmento *b = (segmento*) elemento;
		return !strcmp(b->tabla , nombre_tabla_buscada);
	}
	segmento *segmento = list_find(TABLA_DE_SEGMENTOS, &_buscar_segmento_en_memoria);
	return segmento;
}

int encontrar_pagina_vacia(int tamanio_memoria,int tamanio_maximo_pagina){
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

