#ifndef METRICAS_H_
#define METRICAS_H_

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <commons/collections/list.h>

typedef enum {
	OP_SELECT, OP_INSERT
} tipo_operacion_t;

typedef enum {
	CRITERIO_SC, CRITERIO_SHC, CRITERIO_EC
} criterio_t;

int inicializar_metricas();
int destruir_metricas();

int registrar_operacion(tipo_operacion_t tipo, criterio_t criterio,
		uint32_t latency, uint16_t id_memoria);
char *obtener_metricas();

#endif
