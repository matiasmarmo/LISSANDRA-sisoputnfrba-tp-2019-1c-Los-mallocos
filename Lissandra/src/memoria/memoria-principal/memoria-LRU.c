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

int LRU() {
	bool _criterio(void *elemento1, void *elemento2) {
		uint64_t tiempo1_sec = ((registro_tabla_pagina*)elemento1)->timestamp_accedido.tv_sec;
		uint64_t tiempo1_usec = ((registro_tabla_pagina*)elemento1)->timestamp_accedido.tv_usec;
		uint64_t tiempo2_sec = ((registro_tabla_pagina*)elemento2)->timestamp_accedido.tv_sec;
		uint64_t tiempo2_usec = ((registro_tabla_pagina*)elemento2)->timestamp_accedido.tv_usec;
		return (tiempo1_sec < tiempo2_sec ||
				(tiempo1_sec == tiempo2_sec && tiempo1_usec <= tiempo2_usec));
	}

	segmento *segmento_reg_LRU = NULL, *segmento_actual = NULL;
	registro_tabla_pagina *registro_LRU = NULL;

	void _verificar_registro(void *elemento) {
		registro_tabla_pagina *reg = (registro_tabla_pagina*) elemento;
		if((registro_LRU == NULL || _criterio(elemento, registro_LRU)) && reg->flag_modificado == 0) {
			registro_LRU = reg;
			segmento_reg_LRU = segmento_actual;
		}
	}

	void _iterar_segmento(void *elemento) {
		segmento_actual = (segmento*) elemento;
		list_iterate(segmento_actual->registro_base, &_verificar_registro);
	}

	bool _es_registro_LRU(void *elemento) {
		return elemento == registro_LRU;
	}

	iterar_tabla_de_segmentos(&_iterar_segmento);

	if(registro_LRU == NULL) {
		return NO_SE_PUEDE_HACER_LRU;
	}

	memoria_log_to_level(LOG_LEVEL_INFO, 0, 
		"Reemplazando página por LRU. Tabla: %s, key: %d",
		segmento_reg_LRU->tabla, *((uint16_t*)(registro_LRU->puntero_a_pagina)));

	list_remove_by_condition(segmento_reg_LRU->registro_base, &_es_registro_LRU);
	destruir_registro_de_pagina(registro_LRU);
	return LRU_OK;
}