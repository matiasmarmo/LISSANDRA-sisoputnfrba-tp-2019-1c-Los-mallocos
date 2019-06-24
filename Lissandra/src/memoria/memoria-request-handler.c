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
	lugar_pagina_vacia = encontrar_pagina_vacia();
	if (lugar_pagina_vacia == -1) {
		// INTENTAR HACER LRU
		// SI NO PUEDO, HACER JOURNAL
	}
	// PEDIRSELO AL FS
	crear_segmento_nuevo("TABLA_4");
	segmento* segmento_actual = encontrar_segmento_en_memoria("TABLA_4");
	crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
			segmento_actual, flag_modificado,timestamp_accedido);
	crear_pagina_nueva(lugar_pagina_vacia, 25, 123456, "martin");
	int aux = init_select_response(0, "tablaPrueba", 25, "martin",
			123456, &respuesta);
	if (aux < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Falló la respuesta del SELECT, error en init_select_response()");
		return ERROR;
	}
	memcpy(respuesta_select, &respuesta,sizeof(struct select_response));
	return 0;
}

int _manejar_select_pagina_en_memoria(struct select_request mensaje, void* respuesta_select, segmento* segmento_buscado, registro_tabla_pagina* reg_pagina){
	struct select_response respuesta;
	struct timeval timestamp_accedido;
	gettimeofday(&timestamp_accedido,NULL);
	reg_pagina->timestamp_accedido.tv_sec = timestamp_accedido.tv_sec;
	reg_pagina->timestamp_accedido.tv_usec = timestamp_accedido.tv_usec;
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
		// INTENTAR HACER LRU
		// SI NO PUEDO, HACER JOURNAL
	}
	// PEDIRSELO AL FS
	crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia,
			segmento_buscado, flag_modificado,timestamp_accedido);
	crear_pagina_nueva(lugar_pagina_vacia, 25, 123456, "martin");
	int aux = init_select_response(0, "tablaPrueba", 25, "martin",
			123456, &respuesta);
	if (aux < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Falló la respuesta del SELECT, error en init_select_response()");
		return ERROR;
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
				//PEDIRLE AL FS
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
/*----PARA CHECKPOINT----*/
int _manejar_create(struct create_request mensaje, void* respuesta_create) {
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if (segmento_buscado == NULL) { // No existe el segmento en memoria
		crear_segmento_nuevo(mensaje.tabla);
		memoria_log_to_level(LOG_LEVEL_INFO, false,
				"Creo un nuevo segmento para la tabla %s", mensaje.tabla);
		struct create_response respuesta;
		int aux = init_create_response(0, mensaje.tabla, mensaje.consistencia,
				mensaje.n_particiones, mensaje.t_compactaciones, &respuesta);
		if (aux < 0) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del CREATE, error en init_create_response()");
			return ERROR;
		}
		memcpy(respuesta_create, &respuesta, sizeof(struct create_response));
	} else {
		return ERROR;
	}
	return EXIT_SUCCESS;
}

int _manejar_single_describe(struct describe_request mensaje,
		void* respuesta_describe) {
	struct single_describe_response respuesta;
	if (strcmp(mensaje.tabla, "MARINOS") == 0) {
		init_single_describe_response(0, "MARINOS", 0, 1, 10000, &respuesta);
	} else if (strcmp(mensaje.tabla, "AVES") == 0) {
		init_single_describe_response(0, "AVES", 0, 5, 50000, &respuesta);
	} else if (strcmp(mensaje.tabla, "MAMIFEROS") == 0) {
		init_single_describe_response(0, "MAMIFEROS", 0, 3, 60000, &respuesta);
	} else if (strcmp(mensaje.tabla, "POSTRES") == 0) {
		init_single_describe_response(0, "POSTRES", 0, 3, 60000, &respuesta);
	} else if (strcmp(mensaje.tabla, "FRUTAS") == 0) {
		init_single_describe_response(0, "FRUTAS", 0, 2, 50000, &respuesta);
	} else if (strcmp(mensaje.tabla, "BEBIDAS") == 0) {
		init_single_describe_response(0, "BEBIDAS", 0, 6, 10000, &respuesta);
	} else if (strcmp(mensaje.tabla, "COSAS") == 0) {
		init_single_describe_response(0, "COSAS", 0, 9, 35000, &respuesta);
	} else if (strcmp(mensaje.tabla, "ANIMALES") == 0) {
		init_single_describe_response(0, "ANIMALES", 0, 5, 65000, &respuesta);
	} else if (strcmp(mensaje.tabla, "COLORES") == 0) {
		init_single_describe_response(0, "COLORES", 0, 2, 35000, &respuesta);
	} else if (strcmp(mensaje.tabla, "PLATOS_PRINCIPALES") == 0) {
		init_single_describe_response(0, "PLATOS_PRINCIPALES", 0, 2, 10000,
				&respuesta);
	} else {
		return ERROR;
	}
	memcpy(respuesta_describe, &respuesta,
			sizeof(struct single_describe_response));
	return EXIT_SUCCESS;
}

int _manejar_global_describe(struct describe_request mensaje, void *respuesta) {
	char *nombres_tablas =
			"MARINOS;AVES;MAMIFEROS;POSTRES;FRUTAS;BEBIDAS;COSAS;ANIMALES;COLORES;PLATOS_PRINCIPALES";
	uint8_t consistencias[10] = { 0 };
	uint8_t n_particiones[10] = { 1, 5, 3, 3, 2, 6, 9, 5, 2, 2 };
	uint32_t t_compactaciones[10] = { 10000, 50000, 60000, 60000, 50000, 10000,
			35000, 65000, 35000, 10000 };
	init_global_describe_response(0, nombres_tablas, 10, consistencias, 10,
			n_particiones, 10, t_compactaciones, respuesta);
	return 0;
}

int _manejar_describe(struct describe_request mensaje, void *respuesta) {
	if (mensaje.todas) {
		return _manejar_global_describe(mensaje, respuesta);
	}
	return _manejar_single_describe(mensaje, respuesta);
}

int _manejar_gossip(struct gossip mensaje, void *respuesta) {
	return obtener_respuesta_gossip(respuesta);
}
