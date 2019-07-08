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
#include "memoria-request-handler.h"
#include "memoria-gossip.h"
#include "memoria-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"
#include "memoria-LRU.h"
#include "conexion-lfs.h"

int _manejar_insert_no_existe_segmento_en_tabla(struct insert_request mensaje, void* respuesta_insert);
int _manejar_insert_crear_segmento_nuevo(struct insert_request mensaje, void* respuesta_insert, int lugar_pagina_vacia);
int _manejar_insert_existe_segmento_en_tabla(struct insert_request mensaje, void* respuesta_insert,segmento* segmento_buscado);
int _insert_pagina_en_memoria(struct insert_request mensaje, void* respuesta_insert,segmento* segmento_buscado, registro_tabla_pagina* reg_pagina);
int _insert_pagina_no_en_memoria(struct insert_request mensaje, void* respuesta_insert,segmento* segmento_buscado);

int _manejar_insert(struct insert_request mensaje, void* respuesta_insert) {
	string_to_upper(mensaje.tabla);
	if(strlen(mensaje.valor)>get_tamanio_value()){
		struct error_msg resp;
		int aux = init_error_msg(resp.error_code,"El tamaño del value supera el tamaño maximo permitido",&resp);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Error en init_error_msg()");
			return ERROR;
		}
		memcpy(respuesta_insert, &resp, sizeof(struct error_msg));
		return 0;
	}
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if (segmento_buscado == NULL) { // No existe el segmento en memoria
		int aux = _manejar_insert_no_existe_segmento_en_tabla(mensaje,
															respuesta_insert);
		return aux;
	} else { // El segmento esta en memoria
		int aux = _manejar_insert_existe_segmento_en_tabla(mensaje,
															respuesta_insert,
															segmento_buscado);
		return aux;
	}
}

int _manejar_insert_no_existe_segmento_en_tabla(struct insert_request mensaje, void* respuesta_insert){
	int lugar_pagina_vacia;
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
			memcpy(respuesta_insert, &memoria_full, sizeof(struct memory_full));
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"No se pudo hacer el LRU, se devolvio memory full");
			return 0;
		}
		else{
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Se pudo hacer el LRU --> la pagina quitada se encontraba en la posicion de memoria: %d", encontrar_pagina_vacia());
			lugar_pagina_vacia = encontrar_pagina_vacia();
		}
	} else {
		_manejar_insert_crear_segmento_nuevo(mensaje, respuesta_insert, lugar_pagina_vacia);
	}
	return 0;
}

int _manejar_insert_crear_segmento_nuevo(struct insert_request mensaje, void* respuesta_insert, int lugar_pagina_vacia) {
	int flag_modificado = 1;
	struct insert_response respuesta;
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	crear_segmento_nuevo(mensaje.tabla);
	segmento* segmento_actual = encontrar_segmento_en_memoria(mensaje.tabla);
	crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
			segmento_actual, flag_modificado,timestamp_accedido);
	crear_pagina_nueva(lugar_pagina_vacia, mensaje.key,
		mensaje.timestamp, mensaje.valor);
	memoria_log_to_level(LOG_LEVEL_TRACE, false,
			"Se agrego correctamente la tabla \"%s\" que no estaba en memoria --> y a ella la pagina que posee key = %d y valor = \"%s\".",mensaje.tabla,mensaje.key,mensaje.valor);

	if(init_insert_response(0, mensaje.tabla, mensaje.key, mensaje.valor, mensaje.timestamp, &respuesta) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Falló la respuesta del SELECT, error en init_select_response()");
		return ERROR;
	}
	memcpy(respuesta_insert, &respuesta,sizeof(struct insert_request));
	return 0;
}

int _manejar_insert_existe_segmento_en_tabla(struct insert_request mensaje, void* respuesta_insert,segmento* segmento_buscado){
	registro_tabla_pagina* reg_pagina = encontrar_pagina_en_memoria(
		segmento_buscado, mensaje.key);
	if (reg_pagina == NULL) { // No existe la pagina en memoria
		_insert_pagina_no_en_memoria(mensaje, respuesta_insert, segmento_buscado);
	} else { // La pagina esta en memoria
		_insert_pagina_en_memoria(mensaje, respuesta_insert, segmento_buscado, reg_pagina);
	}
	return 0;
}

int _insert_pagina_en_memoria(struct insert_request mensaje, void* respuesta_insert,segmento* segmento_buscado, registro_tabla_pagina* reg_pagina) {
	memoria_log_to_level(LOG_LEVEL_INFO, false,
			"La key %d de la tabla %s se encuentra en memoria, la actualizo",
			mensaje.key, segmento_buscado->tabla);
	reg_pagina->flag_modificado = 1;
	*((uint16_t*) (reg_pagina->puntero_a_pagina)) = mensaje.key;
	*((uint64_t*) (reg_pagina->puntero_a_pagina + 2)) = mensaje.timestamp;
	memset(reg_pagina->puntero_a_pagina + 10, 0,
			get_tamanio_maximo_pagina());
	memcpy((char*) (reg_pagina->puntero_a_pagina + 10), mensaje.valor,
			strlen(mensaje.valor));
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	reg_pagina->timestamp_accedido.tv_sec = timestamp_accedido.tv_sec;
	reg_pagina->timestamp_accedido.tv_usec = timestamp_accedido.tv_usec;
	struct insert_response respuesta;
	if(init_insert_response(0, mensaje.tabla, mensaje.key, mensaje.valor, mensaje.timestamp, &respuesta) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Falló la respuesta del SELECT, error en init_select_response()");
		return ERROR;
	}
	memcpy(respuesta_insert, &respuesta,sizeof(struct insert_request));
	return 0;
}

int _insert_pagina_no_en_memoria(struct insert_request mensaje, void* respuesta_insert,segmento* segmento_buscado) {
	int lugar_pagina_vacia;
	int flag_modificado = 1;
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	memoria_log_to_level(LOG_LEVEL_INFO, false,
			"La key %d de la tabla %s no se encuentra en memoria, creo una pagina nueva",
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
			memcpy(respuesta_insert, &memoria_full, sizeof(struct memory_full));
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"No se pudo hacer el LRU, se devolvio memory full");
			return 0;
		}
		else{
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Se pudo hacer el LRU --> la pagina quitada se encontraba en la posicion de memoria: %d", encontrar_pagina_vacia());
			lugar_pagina_vacia = encontrar_pagina_vacia();
		}
	} else {
		crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
				segmento_buscado, flag_modificado,timestamp_accedido);
		crear_pagina_nueva(lugar_pagina_vacia, mensaje.key,
				mensaje.timestamp, mensaje.valor);
		struct insert_response respuesta;
		if(init_insert_response(0, mensaje.tabla, mensaje.key, mensaje.valor, mensaje.timestamp, &respuesta) < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del SELECT, error en init_select_response()");
			return ERROR;
		}
		memcpy(respuesta_insert, &respuesta,sizeof(struct insert_request));
	}
	return 0;
}
