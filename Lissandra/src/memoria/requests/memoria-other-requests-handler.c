#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>
#include <sys/time.h>
#include <commons/string.h>

#include "../../commons/comunicacion/protocol.h"
#include "../../commons/lissandra-threads.h"
#include "../../commons/parser.h"
#include "memoria-other-requests-handler.h"
#include "../memoria-gossip.h"
#include "../memoria-principal/memoria-handler.h"
#include "../memoria-logger.h"
#include "../memoria-config.h"
#include "../memoria-server.h"
#include "../memoria-main.h"
#include "../memoria-principal/memoria-LRU.h"
#include "../conexion-lfs.h"

int _manejar_create(struct create_request mensaje, void* respuesta_create, bool should_sleep) {
	return enviar_mensaje_lfs(&mensaje, respuesta_create, should_sleep);
}

int _manejar_describe(struct describe_request mensaje, void *respuesta, bool should_sleep) {
	return enviar_mensaje_lfs(&mensaje, respuesta, should_sleep);
}

int _manejar_gossip(struct gossip mensaje, void *respuesta) {
	if (mensaje.ip_sender != 0){
		agregar_memoria_a_tabla_gossip(mensaje.ip_sender, mensaje.puerto_sender, mensaje.numero_mem_sender);
	}
	return obtener_respuesta_gossip(respuesta);
}

int _manejar_drop(struct drop_request mensaje, void *respuesta, bool should_sleep) {
	string_to_upper(mensaje.tabla);
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if (segmento_buscado != NULL) { // No existe el segmento en memoria
		destruccion_segmento(segmento_buscado);
	}
	return enviar_mensaje_lfs(&mensaje, respuesta, should_sleep);
}
