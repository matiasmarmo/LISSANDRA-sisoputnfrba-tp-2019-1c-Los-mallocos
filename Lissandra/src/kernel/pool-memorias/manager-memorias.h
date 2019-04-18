#ifndef KERNEL_MANAGER_MEMORIAS_H_
#define KERNEL_MANAGER_MEMORIAS_H_

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../commons/comunicacion/protocol.h"

typedef struct memoria {
	uint16_t id_memoria;
	char ip_memoria[16];
	char puerto_memoria[8];
	pthread_mutex_t mutex;
	int socket_fd;
	bool conectada;
} memoria_t;

int inicializar_memorias();
int actualizar_memorias();
int destruir_memorias();

int agregar_memoria_a_criterio(uint16_t, uint8_t);

int realizar_describe(struct global_describe_response*);

int realizar_request(void*, void*, int);

#endif
