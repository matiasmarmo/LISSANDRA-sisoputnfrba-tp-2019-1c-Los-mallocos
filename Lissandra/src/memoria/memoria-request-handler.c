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
#include "../commons/parser.h"
#include "memoria-request-handler.h"
#include "memoria-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"


int _manejar_select(struct select_request mensaje, void* respuesta_select){
	int lugar_pagina_vacia;
	int flag_modificado = 0;
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if(segmento_buscado==NULL){ // No existe el segmento en memoria
		//printf("   El segmento no esta en memoria\n");
		lugar_pagina_vacia = encontrar_pagina_vacia();
		if(lugar_pagina_vacia == -1){
			// HACER JOURNAL
		}else{
			// PEDIRSELO AL FS
			//   me da un struct con el mensaje. Ej.: "[TABLA_4 | KEY= 25 | VALUE= "martin"]"
			crear_segmento_nuevo("TABLA_4");
			segmento* segmento_actual = encontrar_segmento_en_memoria("TABLA_4");
			crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia, segmento_actual, flag_modificado);
			// TENGO QUE OBTENER EL TIMESTAMP DE ALGUNA FORMA...
			crear_pagina_nueva(lugar_pagina_vacia, 25, 123456, "martin");
			// DEVUELVO LA STRUCT QUE ME PASARON
			// FIN
		}
	}else{ // El segmento esta en memoria
		registro_tabla_pagina* reg_pagina = encontrar_pagina_en_memoria(segmento_buscado, mensaje.key);
		if(reg_pagina==NULL){ // No existe la pagina en memoria
			memoria_log_to_level(LOG_LEVEL_INFO, false,
					"La key %d de la tabla %s no se encuentra en memoria, voy a pedirla al FS", mensaje.key, segmento_buscado->tabla);
			lugar_pagina_vacia = encontrar_pagina_vacia();
			if(lugar_pagina_vacia == -1){
				// HACER JOURNAL
			}else{
				// PEDIRSELO AL FS
				//   me da un struct con el mensaje. Ej.: "[TABLA_4 | KEY= 25 | VALUE= "martin"]"
				crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia, segmento_buscado, flag_modificado);
				// TENGO QUE OBTENER EL TIMESTAMP DE ALGUNA FORMA...
				crear_pagina_nueva(lugar_pagina_vacia, 25, 123456, "martin");
				struct select_response respuesta;
				int aux = init_select_response(0,"tablaPrueba",25,"martin",123456,&respuesta);
				if(aux < 0){
					memoria_log_to_level(LOG_LEVEL_TRACE, false,
							"Falló la respuesta del SELECT, error en init_select_response()");
					return ERROR;
				}
				memcpy(respuesta_select, &respuesta, sizeof(struct select_response));
			}
		}
		else{ // La pagina esta en memoria
			struct select_response respuesta;
			int aux = init_select_response(0,segmento_buscado->tabla,*((uint16_t*)(reg_pagina->puntero_a_pagina)),(char*)(reg_pagina->puntero_a_pagina + 10 ),*((uint64_t*)(reg_pagina->puntero_a_pagina + 2)),&respuesta);
			if(aux < 0){
				memoria_log_to_level(LOG_LEVEL_TRACE, false,
								"Falló la respuesta del SELECT, error en init_select_response()");
				return ERROR;
			}
			memcpy(respuesta_select, &respuesta, sizeof(struct select_response));
		}
	}
	return EXIT_SUCCESS;
}

int _manejar_insert(struct insert_request mensaje, void* respuesta_insert){
	int lugar_pagina_vacia;
	int flag_modificado = 0;
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if(segmento_buscado==NULL){ // No existe el segmento en memoria
		//printf("   El segmento no esta en memoria\n");
		lugar_pagina_vacia = encontrar_pagina_vacia();
		if(lugar_pagina_vacia == -1){
			// HACER JOURNAL
		}else{
			crear_segmento_nuevo(mensaje.tabla);
			segmento* segmento_actual = encontrar_segmento_en_memoria(mensaje.tabla);
			crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia, segmento_actual, flag_modificado);
			crear_pagina_nueva(lugar_pagina_vacia, mensaje.key, mensaje.timestamp, mensaje.valor);
			// DEVUELVO LA STRUCT QUE ME PASARON
			// FIN
		}
	}else{ // El segmento esta en memoria
		registro_tabla_pagina* reg_pagina = encontrar_pagina_en_memoria(segmento_buscado, mensaje.key);
		if(reg_pagina==NULL){ // No existe la pagina en memoria
			memoria_log_to_level(LOG_LEVEL_INFO, false,
					"La key %d de la tabla %s no se encuentra en memoria, creo una pagina nueva", mensaje.key, segmento_buscado->tabla);
			lugar_pagina_vacia = encontrar_pagina_vacia();
			if(lugar_pagina_vacia == -1){
				memoria_log_to_level(LOG_LEVEL_INFO, false,
						"La memoria esta FULL, hago un JOURNAL");
				// HACER JOURNAL
			}else{
				//PEDIRLE AL FS
				crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia, segmento_buscado, flag_modificado);
				crear_pagina_nueva(lugar_pagina_vacia, mensaje.key, mensaje.timestamp, mensaje.valor);
				// DEVUELVO LA STRUCT QUE ME PASARON
				// FIN
			}
		}
		else{ // La pagina esta en memoria
			memoria_log_to_level(LOG_LEVEL_INFO, false,
								"La key %d de la tabla %s se encuentra en memoria, la actualizo", mensaje.key, segmento_buscado->tabla);
			*((uint16_t*)(reg_pagina->puntero_a_pagina)) = mensaje.key;
			*((uint64_t*)(reg_pagina->puntero_a_pagina + 2)) = mensaje.timestamp;
			memcpy((char*)(reg_pagina->puntero_a_pagina + 10) , mensaje.valor,10);
		}
	}
	init_insert_response(0, mensaje.tabla, mensaje.key, mensaje.valor, mensaje.timestamp, respuesta_insert);
	return EXIT_SUCCESS;
}
/*----PARA CHECKPOINT----*/
int _manejar_create(struct create_request mensaje, void* respuesta_create){
	segmento* segmento_buscado = encontrar_segmento_en_memoria(mensaje.tabla);
	if(segmento_buscado==NULL){ // No existe el segmento en memoria
		crear_segmento_nuevo(mensaje.tabla);
		memoria_log_to_level(LOG_LEVEL_INFO, false,
							"Creo un nuevo segmento para la tabla %s", mensaje.tabla);
		struct create_response respuesta;
		int aux = init_create_response(0,mensaje.tabla,mensaje.consistencia,
				        mensaje.n_particiones,mensaje.t_compactaciones,&respuesta);
		if(aux < 0){
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Falló la respuesta del CREATE, error en init_create_response()");
			return ERROR;
		}
		memcpy(respuesta_create, &respuesta, sizeof(struct create_response));
	}else{
		return ERROR;
	}
	return EXIT_SUCCESS;
}
