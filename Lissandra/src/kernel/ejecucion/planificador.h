#ifndef KERNEL_PLANIFICADOR_H_
#define KERNEL_PLANIFICADOR_H_

#include <stdbool.h>

typedef struct nuevo_script {
	// Si es un request unitario, str contiene el request.
	// Si no es un request unitario, str contiene el path al archivo LQL
	bool es_request_unitario;
	char *str;
} nuevo_script_t;

typedef enum {
	NUEVO, RUNNING, SCRIPT_FINALIZADO, INT_FIN_QUANTUM, ERROR_SCRIPT, ERROR_MISC
} estado_runner_thread_t;

typedef struct script_control_block {
	// Si es un request unitario, deberá mostrarse
	// el resultado de la ejecución por la consola
	bool es_request_unitario;
	char *source;
	int request_pointer;
	estado_runner_thread_t estado;
} SCB;

int agregar_nuevo_script(bool, char*);

void *correr_planificador(void*);

#endif
