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

t_list* TABLA_DE_GOSSIP;

void inicializacion_tabla_segmentos(){
	TABLA_DE_GOSSIP = list_create();
}

void destruccion_tabla_segmentos(){
	void _destroy_element(void *elemento) {
		free((tabla_gossip*) elemento);
	}
	list_destroy_and_destroy_elements(TABLA_DE_GOSSIP,&_destroy_element);
}


