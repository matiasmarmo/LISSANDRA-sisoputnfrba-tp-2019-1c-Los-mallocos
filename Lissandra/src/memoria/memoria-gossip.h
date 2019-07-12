#ifndef LISSANDRA_SRC_MEMORIA_MEMORIA_GOSSIP_H_
#define LISSANDRA_SRC_MEMORIA_MEMORIA_GOSSIP_H_

#include <stdint.h>

typedef struct memoria_gossip {
	uint32_t ip_memoria;
	uint16_t puerto_memoria;
	uint8_t numero_memoria;
} memoria_gossip_t;

int inicializacion_tabla_gossip();
void destruccion_tabla_gossip();
int agregar_memoria_a_tabla_gossip(uint32_t, uint16_t, uint8_t);
int agregar_esta_memoria_a_tabla_de_gossip();
int obtener_respuesta_gossip(void *);
void *realizar_gossip_threaded(void *entrada);

#endif
