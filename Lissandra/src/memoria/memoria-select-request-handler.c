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

#include "../commons/comunicacion/protocol.h"
#include "../commons/lissandra-threads.h"
#include "../commons/parser.h"
#include "memoria-gossip.h"
#include "memoria-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"
#include "memoria-LRU.h"
#include "conexion-lfs.h"
#include "memoria-other-requests-handler.h"


int _manejar_select_segmento_encontrado(struct select_request, void*,segmento*);
int _manejar_select_segmento_no_encontrado(struct select_request, void*,segmento*);
int _manejar_select_pagina_en_memoria(struct select_request, void*,segmento*,registro_tabla_pagina*);
int _manejar_select_pagina_no_en_memoria(struct select_request, void*,segmento*);

int _manejar_select(struct select_request mensaje, void* respuesta_select) {
	string_to_upper(mensaje.tabla);
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if (segmento_buscado == NULL) { // No existe el segmento en memoria
		int a = _manejar_select_segmento_no_encontrado(mensaje,
													respuesta_select,
													segmento_buscado);
		return a;
	} else { // El segmento esta en memoria
		int a = _manejar_select_segmento_encontrado(mensaje,
													respuesta_select,
													segmento_buscado);
		return a;
	}
	return EXIT_SUCCESS;
}

int _manejar_select_segmento_encontrado(struct select_request mensaje, void* respuesta_select, segmento* segmento_buscado){
	registro_tabla_pagina* reg_pagina = encontrar_pagina_en_memoria(
									segmento_buscado, mensaje.key);
	if (reg_pagina == NULL) { // No existe la pagina en memoria
		int aux = _manejar_select_pagina_no_en_memoria(mensaje,
													 respuesta_select,
													 segmento_buscado);
		return aux;
	} else { // La pagina esta en memoria
		int aux = _manejar_select_pagina_en_memoria(mensaje,
												  respuesta_select,
												  segmento_buscado,
												  reg_pagina);
		return aux;
	}
}

int _manejar_select_segmento_no_encontrado(struct select_request mensaje, void* respuesta_select,segmento* segmento_buscado){
	int lugar_pagina_vacia;
	int flag_modificado = 0;
	uint8_t respuesta_lfs[get_max_msg_size()];
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	memoria_log_to_level(LOG_LEVEL_TRACE, false,
			"La tabla \"%s\" no esta en memoria, se le pide al FS",mensaje.tabla);
	lugar_pagina_vacia = encontrar_pagina_vacia();
	if (lugar_pagina_vacia == -1) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"No hay espacio en memoria para una nueva pagina, se intentara hacer LRU");
		/*----INTENTO HACER LRU----*/
		if(LRU()<0){
			struct memory_full memoria_full;
			int aux = init_memory_full(&memoria_full);
			if (aux < 0) {
				memoria_log_to_level(LOG_LEVEL_TRACE, false,"Falló el init_memory_full()");
				return ERROR;
			}
			memcpy(respuesta_select, &memoria_full, sizeof(struct memory_full));
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
							"No se pudo hacer el LRU, se devolvio memory full");
			return 0;
		}
		else{
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Se pudo hacer el LRU --> la pagina quitada se encontraba en la posicion de memoria: %d", encontrar_pagina_vacia());
			lugar_pagina_vacia = encontrar_pagina_vacia();
		}
	}
	/*----PEDIDO AL FS----*/
	if(enviar_mensaje_lfs(&mensaje, &respuesta_lfs) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo la comunicacion con FS para pedirle una pagina");
		return ERROR;
	}
	if(get_msg_id(&respuesta_lfs) != ERROR_MSG_ID){
		struct select_response resp = *((struct select_response*) respuesta_lfs);
		crear_segmento_nuevo(resp.tabla);
		segmento* segmento_actual = encontrar_segmento_en_memoria(resp.tabla);
		crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
				segmento_actual, flag_modificado,timestamp_accedido);
		crear_pagina_nueva(lugar_pagina_vacia, resp.key, resp.timestamp, resp.valor);
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Se agrego correctamente a memoria la tabla \"%s\", y la pagina que posee key = %d y valor = \"%s\".",mensaje.tabla,resp.key,resp.valor);
		int aux = init_select_response(0, resp.tabla, resp.key, resp.valor,
				resp.timestamp, &resp);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del SELECT, error en init_select_response()");
			return ERROR;
		}
		memcpy(respuesta_select, &resp, sizeof(struct select_response));
	}
	else{
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"No se pudo traer a memoria --> la pagina que posee key = %d de la tabla \"%s\", dado que esta no existe en el FS",mensaje.key,mensaje.tabla);
		struct error_msg resp = *((struct error_msg*) respuesta_lfs);
		int aux = init_error_msg(resp.error_code,resp.description,&resp);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del SELECT, error en init_error_response()");
			return ERROR;
		}
		memcpy(respuesta_select, &resp, sizeof(struct error_msg));
	}
	return 0;
}

int _manejar_select_pagina_en_memoria(struct select_request mensaje, void* respuesta_select, segmento* segmento_buscado, registro_tabla_pagina* reg_pagina){
	struct select_response respuesta;
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	reg_pagina->timestamp_accedido.tv_sec = timestamp_accedido.tv_sec;
	reg_pagina->timestamp_accedido.tv_usec = timestamp_accedido.tv_usec;
	memoria_log_to_level(LOG_LEVEL_TRACE, false,
			"Se solicitó la key = %d de la tabla \"%s\" que se encontraba en memoria.",mensaje.key,mensaje.tabla);
	int aux = init_select_response(0, segmento_buscado->tabla,
			*((uint16_t*) (reg_pagina->puntero_a_pagina)),
			(char*) (reg_pagina->puntero_a_pagina + 10),
			*((uint64_t*) (reg_pagina->puntero_a_pagina + 2)),
			&respuesta);
	if (aux < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Falló la respuesta del SELECT, error en init_select_response()");
		return ERROR;
	}
	memcpy(respuesta_select, &respuesta,sizeof(struct select_response));
	return 0;
}

int _manejar_select_pagina_no_en_memoria(struct select_request mensaje, void* respuesta_select,segmento* segmento_buscado){
	uint8_t respuesta_lfs[get_max_msg_size()];
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	int lugar_pagina_vacia = 0;
	int flag_modificado = 0;
	memoria_log_to_level(LOG_LEVEL_INFO, false,
			"La key %d de la tabla %s no se encuentra en memoria, voy a pedirla al FS",
			mensaje.key, segmento_buscado->tabla);
	lugar_pagina_vacia = encontrar_pagina_vacia();
	if (lugar_pagina_vacia == -1) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"No hay espacio en memoria para una nueva pagina, se intentara hacer LRU");
		/*----INTENTO HACER LRU----*/
		if(LRU()<0){
			struct memory_full memoria_full;
			int aux = init_memory_full(&memoria_full);
			if (aux < 0) {
				memoria_log_to_level(LOG_LEVEL_TRACE, false,"Falló el init_memory_full()");
				return ERROR;
			}
			memcpy(respuesta_select, &memoria_full, sizeof(struct memory_full));
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"No se pudo hacer el LRU, se devolvio memory full");
			return 0;
		}
		else{
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Se pudo hacer el LRU --> la pagina quitada se encontraba en la posicion de memoria: %d", encontrar_pagina_vacia());
			lugar_pagina_vacia = encontrar_pagina_vacia();
		}
	}
	/*----PEDIDO AL FS----*/
	if(enviar_mensaje_lfs(&mensaje, &respuesta_lfs) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo la comunicacion con FS para pedirle una pagina");
		return ERROR;
	}
	if(get_msg_id(&respuesta_lfs) != ERROR_MSG_ID){
		struct select_response resp = *((struct select_response*) respuesta_lfs);
		crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
				segmento_buscado, flag_modificado,timestamp_accedido);
		crear_pagina_nueva(lugar_pagina_vacia, resp.key, resp.timestamp, resp.valor);
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Se agrego correctamente a la tabla \"%s\" --> la pagina que posee key = %d y valor = \"%s\".",mensaje.tabla,resp.key,resp.valor);
		int aux = init_select_response(0, segmento_buscado->tabla, resp.key, resp.valor,
				resp.timestamp, &resp);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del SELECT, error en init_select_response()");
			return ERROR;
		}
		memcpy(respuesta_select, &resp,sizeof(struct select_response));
	}
	else{
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"No se pudo traer a memoria --> la pagina que posee key = %d de la tabla \"%s\", dado que esta no existe en el FS",mensaje.key,mensaje.tabla);
		struct error_msg resp = *((struct error_msg*) respuesta_lfs);
		int aux = init_error_msg(resp.error_code,resp.description,&resp);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del SELECT, error en init_error_response()");
			return ERROR;
		}
		memcpy(respuesta_select, &resp, sizeof(struct error_msg));
	}
	return 0;
}

