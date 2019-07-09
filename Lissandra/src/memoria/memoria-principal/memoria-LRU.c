#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>

#include "../../commons/comunicacion/protocol.h"
#include "../../commons/lissandra-threads.h"
#include "memoria-handler.h"
#include "../memoria-logger.h"
#include "../memoria-config.h"
#include "../memoria-server.h"
#include "../memoria-main.h"
#include "memoria-LRU.h"

registro_tabla_pagina* ultimo_accedido_con_bit_modificado_en_cero_local(t_list* lista_registros){
	int cantidad_registros = list_size(lista_registros);
	registro_tabla_pagina* registro_temporal;
	for(int i = 0 ; i < cantidad_registros ; i++ ){
		registro_temporal = list_get(lista_registros,i);
		if(registro_temporal->flag_modificado == 0){
			return registro_temporal;
		}
	}
	registro_temporal->timestamp_accedido.tv_sec = 0;
	registro_temporal->timestamp_accedido.tv_usec = 0;
	return registro_temporal;
}

registro_tabla_pagina* anteultimo_accedido_con_bit_modificado_en_cero_local(t_list* lista_registros){
	int cantidad_registros = list_size(lista_registros);
	int contador = 0;
	registro_tabla_pagina* registro_temporal;
	for(int i = 0 ; i < cantidad_registros ; i++ ){
		registro_temporal = list_get(lista_registros,i);
		if(registro_temporal->flag_modificado == 0){
			contador++;
			if(contador == 2){
				return registro_temporal;
			}
		}
	}
	registro_temporal->timestamp_accedido.tv_sec = 0;
	registro_temporal->timestamp_accedido.tv_usec = 0;
	return registro_temporal;
}

registro_tabla_pagina* ultimo_accedido_global(registro_tabla_pagina* registros[],int cantidad_registros){
	registro_tabla_pagina* mas_viejo = registros[0];
	for(int i=1;i<cantidad_registros;i++){
		if(registros[i]->timestamp_accedido.tv_sec == mas_viejo->timestamp_accedido.tv_sec
				&& registros[i]->flag_modificado == 0){
			if(registros[i]->timestamp_accedido.tv_usec < mas_viejo->timestamp_accedido.tv_usec
					&& registros[i]->flag_modificado == 0){
				mas_viejo = registros[i];
			}
		}
		else if(registros[i]->timestamp_accedido.tv_sec < mas_viejo->timestamp_accedido.tv_sec
				&& registros[i]->flag_modificado == 0){
			mas_viejo = registros[i];
		}
	}
	return mas_viejo;
}

segmento* obtener_segmento_a_partir_de_registro(registro_tabla_pagina* registro_LRU){
	bool _criterio(void* element){
		return ((registro_tabla_pagina*)element)->timestamp_accedido.tv_sec == registro_LRU->timestamp_accedido.tv_sec &&
			((registro_tabla_pagina*)element)->timestamp_accedido.tv_usec == registro_LRU->timestamp_accedido.tv_usec;
	}
	int cantidad_segmentos = list_size(TABLA_DE_SEGMENTOS);
	segmento* segmento_temporal;
	for(int i=0; i< cantidad_segmentos; i++){
		segmento_temporal = list_get(TABLA_DE_SEGMENTOS,i);
		if(list_any_satisfy(segmento_temporal->registro_base,&_criterio)){
			return segmento_temporal;
		}
	}
	return segmento_temporal;
}

int LRU(){
	bool _criterio(void *elemento1, void *elemento2) {
		uint64_t tiempo1_sec = ((registro_tabla_pagina*)elemento1)->timestamp_accedido.tv_sec;
		uint64_t tiempo1_usec = ((registro_tabla_pagina*)elemento1)->timestamp_accedido.tv_usec;
		uint64_t tiempo2_sec = ((registro_tabla_pagina*)elemento2)->timestamp_accedido.tv_sec;
		uint64_t tiempo2_usec = ((registro_tabla_pagina*)elemento2)->timestamp_accedido.tv_usec;
		return (tiempo1_sec < tiempo2_sec ||
				(tiempo1_sec == tiempo2_sec && tiempo1_usec <= tiempo2_usec));
	}
	segmento* segmento_temporal;
	int cantidad_segmentos = list_size(TABLA_DE_SEGMENTOS);
	registro_tabla_pagina* registro_temporal[cantidad_segmentos*2];
	for(int i=0; i< cantidad_segmentos*2; i=i+2){
		segmento_temporal = list_get(TABLA_DE_SEGMENTOS,i/2);
		list_sort(segmento_temporal->registro_base,&_criterio);
		registro_temporal[i] = ultimo_accedido_con_bit_modificado_en_cero_local(segmento_temporal->registro_base);
		registro_temporal[i+1] = anteultimo_accedido_con_bit_modificado_en_cero_local(segmento_temporal->registro_base);
	}
	registro_tabla_pagina* registro_LRU = ultimo_accedido_global(registro_temporal,cantidad_segmentos*2);
	if(registro_LRU->timestamp_accedido.tv_sec == 0 && registro_LRU->timestamp_accedido.tv_usec == 0){
		memoria_log_to_level(LOG_LEVEL_TRACE,false,
				"LRU: No existen paginas para quitar usando LRU.");
		return NO_SE_PUEDE_HACER_LRU;
	}
	memoria_log_to_level(LOG_LEVEL_TRACE,false,
				"LRU: La pagina quitada tenia la key \"%d\" y el value \"%s\".",*(registro_LRU->puntero_a_pagina),(char*)(registro_LRU->puntero_a_pagina+10));
	destruir_registro_de_pagina(registro_LRU);
	return LRU_OK;
}