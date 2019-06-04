/*
 * memoria-requests-handler.h
 *
 *  Created on: 27 may. 2019
 *      Author: utnso
 */

#ifndef MEMORIA_MEMORIA_REQUESTS_HANDLER_H_
#define MEMORIA_MEMORIA_REQUESTS_HANDLER_H_
#define MAX_TAM_NOMBRE_TABLA 50

typedef struct segmento_t{
	char tabla[MAX_TAM_NOMBRE_TABLA];
	t_list* registro_base;
	int registro_limite;
}segmento;

typedef struct registro_tabla_pagina_t{
	uint16_t numero_pagina;
	uint8_t* puntero_a_pagina; //MARCO
	uint8_t flag_modificado;
}registro_tabla_pagina;

typedef struct pagina_t{
	uint64_t* timestamp;
	uint16_t* key;
	char* value;
}pagina;

void inicializacion_tabla_segmentos();
void destruccion_tabla_segmentos();
void destruccion_tabla_registros_paginas();
void crear_segmento_nuevo(char*);
segmento* encontrar_segmento_en_memoria(char*);
int cantidad_paginas_de_un_segmento(segmento*);
int encontrar_pagina_vacia(uint8_t*,int,int);
void crear_registro_nuevo_en_tabla_de_paginas(uint8_t*, int, segmento*, int);
void crear_pagina_nueva(uint8_t*,int, uint16_t, uint64_t, char*);
registro_tabla_pagina* encontrar_pagina_en_memoria(segmento*, uint16_t);

#endif /* MEMORIA_MEMORIA_REQUESTS_HANDLER_H_ */
