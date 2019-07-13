#ifndef KERNEL_MANAGER_MEMORIAS_H_
#define KERNEL_MANAGER_MEMORIAS_H_

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdbool.h>

#include "../../commons/comunicacion/protocol.h"

typedef enum manager_memorias_error {
	MEMORIA_DESCONOCIDA = -10, TABLA_DESCONOCIDA, FALLO_BLOQUEO_SEMAFORO
} manager_memorias_error_t;

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

void *actualizar_memorias_threaded(void*);

void *keep_alive_threaded(void*);

int agregar_memoria_a_criterio(uint16_t, uint8_t, int);

int realizar_describe(struct global_describe_response*);

int realizar_journal_a_todos_los_criterios();

int enviar_request_a_memoria(void*, void*, int, bool);

#endif
