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

int _manejar_create(struct create_request mensaje, void* respuesta_create) {
	int rta = enviar_mensaje_lfs(&mensaje, respuesta_create);
	if(rta < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Fallo la comunicacion con FS para realizar un create");
		return ERROR;
	}
	return rta;
}

int _manejar_describe(struct describe_request mensaje, void *respuesta) {
	int rta = enviar_mensaje_lfs(&mensaje, respuesta);
	if(rta < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo la comunicacion con FS para realizar un describe");
		return ERROR;
	}
	return rta;
}

int _manejar_gossip(struct gossip mensaje, void *respuesta) {
	int rta = obtener_respuesta_gossip(respuesta);
	if(rta < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo la comunicacion para realizar el gossip");
		return ERROR;
	}
	return rta;
}

////////////////////////////////////////////////////////////

int _manejar_drop(struct drop_request mensaje, void *respuesta) {
	string_to_upper(mensaje.tabla);
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if (segmento_buscado == NULL) { // No existe el segmento en memoria
		int rta = enviar_mensaje_lfs(&mensaje, respuesta);
		if(rta < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Fallo la comunicacion con FS para realizar un drop");
			return ERROR;
		}
		return rta;
	} else { // El segmento esta en memoria
		destruccion_segmento(segmento_buscado);
	}
	return EXIT_SUCCESS;
}
