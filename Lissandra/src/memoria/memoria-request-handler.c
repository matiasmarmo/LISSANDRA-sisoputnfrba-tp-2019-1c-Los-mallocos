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

#include "../commons/comunicacion/protocol.h"
#include "../commons/lissandra-threads.h"
#include "../commons/parser.h"
#include "memoria-request-handler.h"
#include "memoria-gossip.h"
#include "memoria-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"
#include "conexion-lfs.h"


int _manejar_select_segmento_encontrado(struct select_request, void*,segmento*);
int _manejar_select_segmento_no_encontrado(struct select_request, void*,segmento*);
int _manejar_select_pagina_en_memoria(struct select_request, void*,segmento*,registro_tabla_pagina*);
int _manejar_select_pagina_no_en_memoria(struct select_request, void*,segmento*);

int _manejar_select(struct select_request mensaje, void* respuesta_select) {
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
		int a = _manejar_select_pagina_no_en_memoria(mensaje,
													 respuesta_select,
													 segmento_buscado);
		return a;
	} else { // La pagina esta en memoria
		int a = _manejar_select_pagina_en_memoria(mensaje,
												  respuesta_select,
												  segmento_buscado,
												  reg_pagina);
		return a;
	}
	return EXIT_SUCCESS;
}

int _manejar_select_segmento_no_encontrado(struct select_request mensaje, void* respuesta_select,segmento* segmento_buscado){
	int lugar_pagina_vacia;
	int flag_modificado = 0;
	struct select_response respuesta;
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	memoria_log_to_level(LOG_LEVEL_TRACE, false,
			"La tabla \"%s\" no esta en memoria, se le pide al FS",mensaje.tabla);
	lugar_pagina_vacia = encontrar_pagina_vacia();
	if (lugar_pagina_vacia == -1) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"No hay espacio en memoria para una nueva pagina, se intentara hacer LRU");
		// INTENTAR HACER LRU
		// SI NO PUEDO, HACER JOURNAL
	}
	/*----PEDIDO AL FS----*/
	if(enviar_mensaje_lfs(&mensaje, &respuesta) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo la comunicacion con file system para pedirle una pagina");
		return ERROR;
	}
	//if(get_msg_id(&respuesta) != ERROR_MSG_ID){printf("Mensaje OK\n");}
	if(respuesta.fallo == 0){
		crear_segmento_nuevo(respuesta.tabla);
		segmento* segmento_actual = encontrar_segmento_en_memoria(respuesta.tabla);
		crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
				segmento_actual, flag_modificado,timestamp_accedido);
		crear_pagina_nueva(lugar_pagina_vacia, respuesta.key, respuesta.timestamp, respuesta.valor);
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Se agrego correctamente a memoria la tabla \"%s\", y la pagina que posee key = %d y valor = \"%s\".",mensaje.tabla,respuesta.key,respuesta.valor);
		int aux = init_select_response(0, segmento_buscado->tabla, respuesta.key, respuesta.valor,
				respuesta.timestamp, &respuesta);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del SELECT, error en init_select_response()");
			return ERROR;
		}
	}
	memcpy(respuesta_select, &respuesta,
			sizeof(struct select_response));
	return 0;
}

int _manejar_select_pagina_en_memoria(struct select_request mensaje, void* respuesta_select, segmento* segmento_buscado, registro_tabla_pagina* reg_pagina){
	struct select_response respuesta;
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	reg_pagina->timestamp_accedido.tv_sec = timestamp_accedido.tv_sec;
	reg_pagina->timestamp_accedido.tv_usec = timestamp_accedido.tv_usec;
	memoria_log_to_level(LOG_LEVEL_TRACE, false,
			"Se solicitó la key = %d de la tabla \"%s\" que se encontraba en memoria.",respuesta.key,mensaje.tabla);
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
	struct select_response respuesta;
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
		// INTENTAR HACER LRU
		// SI NO PUEDO, HACER JOURNAL
	}
	/*----PEDIDO AL FS----*/
	if(enviar_mensaje_lfs(&mensaje, &respuesta) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo la comunicacion con FS para pedirle una pagina");
		return ERROR;
	}
	//if(get_msg_id(&respuesta) != ERROR_MSG_ID){printf("Mensaje OK\n");}
	if(respuesta.fallo == 0){
		crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
				segmento_buscado, flag_modificado,timestamp_accedido);
		crear_pagina_nueva(lugar_pagina_vacia, respuesta.key, respuesta.timestamp, respuesta.valor);
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Se agrego correctamente a la tabla \"%s\" --> la pagina que posee key = %d y valor = \"%s\".",mensaje.tabla,respuesta.key,respuesta.valor);
		int aux = init_select_response(0, segmento_buscado->tabla, respuesta.key, respuesta.valor,
				respuesta.timestamp, &respuesta);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del SELECT, error en init_select_response()");
			return ERROR;
		}
	}
	memcpy(respuesta_select, &respuesta,
			sizeof(struct select_response));
	return 0;
}

//------------------------------------------------------------
//------------------------------------------------------------

int _manejar_insert(struct insert_request mensaje, void* respuesta_insert) {
	int lugar_pagina_vacia;
	int flag_modificado = 0;
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if (segmento_buscado == NULL) { // No existe el segmento en memoria
		//printf("   El segmento no esta en memoria\n");
		lugar_pagina_vacia = encontrar_pagina_vacia();
		if (lugar_pagina_vacia == -1) {
			// HACER JOURNAL
		} else {
			crear_segmento_nuevo(mensaje.tabla);
			segmento* segmento_actual = encontrar_segmento_en_memoria(
					mensaje.tabla);
			crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
					segmento_actual, flag_modificado,timestamp_accedido);
			memoria_log_to_level(LOG_LEVEL_INFO, false,
											"Tiemstamp insert= %d + %d ", timestamp_accedido.tv_sec, timestamp_accedido.tv_usec);
			crear_pagina_nueva(lugar_pagina_vacia, mensaje.key,
					mensaje.timestamp, mensaje.valor);
			// DEVUELVO LA STRUCT QUE ME PASARON
			// FIN
		}
	} else { // El segmento esta en memoria
		registro_tabla_pagina* reg_pagina = encontrar_pagina_en_memoria(
				segmento_buscado, mensaje.key);
		if (reg_pagina == NULL) { // No existe la pagina en memoria
			memoria_log_to_level(LOG_LEVEL_INFO, false,
					"La key %d de la tabla %s no se encuentra en memoria, creo una pagina nueva",
					mensaje.key, segmento_buscado->tabla);
			lugar_pagina_vacia = encontrar_pagina_vacia();
			if (lugar_pagina_vacia == -1) {
				memoria_log_to_level(LOG_LEVEL_INFO, false,
						"La memoria esta FULL, hago un JOURNAL");
				// HACER JOURNAL
			} else {
				//le pido al fs
				crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
						segmento_buscado, flag_modificado,timestamp_accedido);
				memoria_log_to_level(LOG_LEVEL_INFO, false,
											"Tiemstamp insert= %d + %d ", timestamp_accedido.tv_sec, timestamp_accedido.tv_usec);
				crear_pagina_nueva(lugar_pagina_vacia, mensaje.key,
						mensaje.timestamp, mensaje.valor);
				// DEVUELVO LA STRUCT QUE ME PASARON
				// FIN
			}
		} else { // La pagina esta en memoria
			memoria_log_to_level(LOG_LEVEL_INFO, false,
					"La key %d de la tabla %s se encuentra en memoria, la actualizo",
					mensaje.key, segmento_buscado->tabla);
			*((uint16_t*) (reg_pagina->puntero_a_pagina)) = mensaje.key;
			*((uint64_t*) (reg_pagina->puntero_a_pagina + 2)) =
					mensaje.timestamp;
			memset(reg_pagina->puntero_a_pagina + 10, 0,
					get_tamanio_maximo_pagina());
			memcpy((char*) (reg_pagina->puntero_a_pagina + 10), mensaje.valor,
					strlen(mensaje.valor));
		}
	}
	init_insert_response(0, mensaje.tabla, mensaje.key, mensaje.valor,
			mensaje.timestamp, respuesta_insert);
	return EXIT_SUCCESS;
}

int _manejar_create(struct create_request mensaje, void* respuesta_create) {
	return enviar_mensaje_lfs(&mensaje, respuesta_create);
}

int _manejar_describe(struct describe_request mensaje, void *respuesta) {
	return enviar_mensaje_lfs(&mensaje, respuesta);
}

int _manejar_gossip(struct gossip mensaje, void *respuesta) {
	return obtener_respuesta_gossip(respuesta);
}
