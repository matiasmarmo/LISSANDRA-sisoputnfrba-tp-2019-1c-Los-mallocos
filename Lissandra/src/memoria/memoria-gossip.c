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
#include "../commons/comunicacion/sockets.h"
#include "../commons/lissandra-threads.h"
#include "memoria-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

#include "memoria-gossip.h"

t_list* TABLA_DE_GOSSIP;

void inicializacion_tabla_gossip(){
	TABLA_DE_GOSSIP = list_create();
}

void destruccion_tabla_gossip(){
	void _destroy_element(void *elemento) {
		free((memoria_gossip*) elemento);
	}
	list_destroy_and_destroy_elements(TABLA_DE_GOSSIP,&_destroy_element);
}

int agregar_memoria_a_tabla_gossip(){
	memoria_gossip* nueva_memoria = malloc(sizeof(memoria_gossip));
	char *puerto_en_char = malloc(sizeof(uint16_t));
	int *puerto_en_int = malloc(sizeof(uint16_t));
	if (puerto_en_char == NULL) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
			   "Construcción de puerto memoria fallida, error en malloc");
		return NULL;
	}
	if(get_ip_propia(puerto_en_char, 16) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Error al obtener mi puerto");
		return NULL;
	}
	if(get_ip_propia(puerto_en_char, puerto_en_int) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Error al convertir mi puerto a un int");
		return NULL;
	}
	nueva_memoria->ip_memoria = &puerto_en_int;
	nueva_memoria->puerto_memoria = get_puerto_escucha_mem();
	nueva_memoria->numero_memoria = get_numero_memoria();
	list_add(TABLA_DE_GOSSIP, nueva_memoria);
	return 1;
}

int realizar_gossip(uint8_t id){ // devuelvo las memorias que conozcoint lugar_pagina_vacia;	int lugar_pagina_vacia;
	memoria_gossip* recorrer = malloc(sizeof(memoria_gossip));
	uint32_t ips[list_size(TABLA_DE_SEGMENTOS)];
	uint16_t puertos[list_size(TABLA_DE_SEGMENTOS)];
	uint8_t numeros[list_size(TABLA_DE_SEGMENTOS)];
	for(int i = 0; i < list_size(TABLA_DE_SEGMENTOS) - 1; i++){
		recorrer = (TABLA_DE_GOSSIP, i);
		ips[i] = recorrer->ip_memoria;
		puertos[i] = recorrer->puerto_memoria;
		numeros[i] = recorrer->numero_memoria;
	}

	struct gossip_response respuesta;
	int aux = init_gossip_response(list_size(TABLA_DE_SEGMENTOS), ips,
				list_size(TABLA_DE_SEGMENTOS), puertos,
					list_size(TABLA_DE_SEGMENTOS), numeros, &respuesta);
	if(aux < 0){
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Error al tratar de responder al gossip");
		return NULL;
	}
	if(enviar_respuesta_gossip(respuesta) < 0){
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Error al enviar respuesta gossip");
		return NULL;
	}
	return EXIT_SUCCESS;
}



