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
	registro_temporal->timestamp_accedido = 0;
	return registro_temporal;
}

registro_tabla_pagina* ultimo_accedido_global(registro_tabla_pagina* registros[],int cantidad_registros){
	registro_tabla_pagina* maximo = registros[0];
	for(int i=0;i<cantidad_registros;i++){
		if(registros[i]->timestamp_accedido > maximo->timestamp_accedido
				&& registros[i]->flag_modificado == 0){
			maximo = registros[i];
		}
	}
	return maximo;
}

int LRU(){
	bool _criterio(void *elemento1, void *elemento2) {
		uint64_t tiempo1 = ((registro_tabla_pagina*)elemento1)->timestamp_accedido;
		uint64_t tiempo2 = ((registro_tabla_pagina*)elemento2)->timestamp_accedido;
		return tiempo1 <= tiempo2;
	}
	segmento* segmento_temporal;
	int cantidad_segmentos = list_size(TABLA_DE_SEGMENTOS);
	registro_tabla_pagina* registro_temporal[cantidad_segmentos];
	for(int i=0; i< cantidad_segmentos; i++){
		segmento_temporal = list_get(TABLA_DE_SEGMENTOS,i);
		list_sort(segmento_temporal->registro_base,&_criterio);
		registro_temporal[i] = ultimo_accedido_con_bit_modificado_en_cero_local(segmento_temporal->registro_base);
	}
	registro_tabla_pagina* registro_LRU = ultimo_accedido_global(registro_temporal,cantidad_segmentos);
	if(registro_LRU->timestamp_accedido == 0){
		return -1;
	}
	return 0;
}




