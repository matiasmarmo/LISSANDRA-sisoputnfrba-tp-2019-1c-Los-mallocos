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

void agregar_memoria_a_tabla_gossip(){
	memoria_gossip* nueva_memoria = malloc(sizeof(memoria_gossip));
	//nueva_memoria->ip_memoria = "127.0.0.1"; // usar getaddrinfo y getnameinfo para obtener ip propia
	nueva_memoria->puerto_memoria = get_puerto_escucha_mem();
	nueva_memoria->numero_memoria = get_numero_memoria();
	list_add(TABLA_DE_GOSSIP, nueva_memoria);
}

int realizar_gossip(struct gossip mensaje, void* respuesta_gossip){ // devuelvo las memorias que conozcoint lugar_pagina_vacia;	int lugar_pagina_vacia;

	return EXIT_SUCCESS;
}



