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
		//printf("segmento_buscado -> reg_lim: %d\n",segmento_buscado->registro_limite);
		//printf("segmento_buscado -> nombre: %s\n",segmento_buscado->tabla);
		//cantidad_paginas_de_un_segmento(segmento_buscado);
		registro_tabla_pagina* reg_pagina = encontrar_pagina_en_memoria(segmento_buscado, mensaje.key);
		if(reg_pagina==NULL){ // No existe la pagina en memoria
			//printf("   La pagina con esa key no esta en memoria\n");
			lugar_pagina_vacia = encontrar_pagina_vacia();
			if(lugar_pagina_vacia == -1){
				// HACER JOURNAL
			}else{
				// PEDIRSELO AL FS
				//   me da un struct con el mensaje. Ej.: "[TABLA_4 | KEY= 25 | VALUE= "martin"]"
				crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia, segmento_buscado, flag_modificado);
				// TENGO QUE OBTENER EL TIMESTAMP DE ALGUNA FORMA...
				crear_pagina_nueva(lugar_pagina_vacia, 25, 123456, "martin");
				// DEVUELVO LA STRUCT QUE ME PASARON
				// FIN
			}
		}
		else{ // La pagina esta en memoria
			struct select_response respuesta;
			int aux = init_select_response(0,segmento_buscado->tabla,*((uint16_t*)(reg_pagina->puntero_a_pagina)),(char*)(reg_pagina->puntero_a_pagina + 10 ),(unsigned long)time(NULL),&respuesta);
			if(aux < 0){
				return -1;
			}
			// DEVUELVO STRUCT RESPUESTA
			// FIN
			//printf("   Pagina buscada -> key: %d\n",respuesta.key);
			//printf("   Pagina buscada -> timestamp: %llu\n",respuesta.timestamp);
			//printf("   Pagina buscada -> value: %s\n",respuesta.valor);
			memcpy(respuesta_select, &respuesta, sizeof(struct select_response));
		}
	}
	return 0;
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
			//printf("   La pagina con esa key no esta en memoria\n");
			lugar_pagina_vacia = encontrar_pagina_vacia();
			if(lugar_pagina_vacia == -1){
				//printf("   No hay paginas libres\n");
				// HACER JOURNAL
			}else{
				crear_registro_nuevo_en_tabla_de_paginas(lugar_pagina_vacia, segmento_buscado, flag_modificado);
				crear_pagina_nueva(lugar_pagina_vacia, mensaje.key, mensaje.timestamp, mensaje.valor);
				// DEVUELVO LA STRUCT QUE ME PASARON
				// FIN
			}
		}
		else{ // La pagina esta en memoria
			//struct insert_response respuesta;
			//int aux = init_select_response(0,segmento_buscado->tabla,*((uint16_t*)(reg_pagina->puntero_a_pagina)),(char*)(reg_pagina->puntero_a_pagina + 10 ),(unsigned long)time(NULL),&respuesta);
			//if(aux < 0){}
			*((uint16_t*)(reg_pagina->puntero_a_pagina)) = mensaje.key;
			*((uint64_t*)(reg_pagina->puntero_a_pagina + 2)) = mensaje.timestamp;
			memcpy((char*)(reg_pagina->puntero_a_pagina + 10) , mensaje.valor,10);
			// DEVUELVO STRUCT RESPUESTA
			// FIN
		}
	}
	init_insert_response(0, mensaje.tabla, mensaje.key, mensaje.valor, mensaje.timestamp, respuesta_insert);
	return 0;
}