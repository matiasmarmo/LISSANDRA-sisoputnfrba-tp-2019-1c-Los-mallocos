#ifndef KERNEL_PLANIFICADOR_H_
#define KERNEL_PLANIFICADOR_H_

typedef struct nuevo_script {
	char *path;
} nuevo_script_t;

typedef enum {
	NUEVO, SCRIPT_FINALIZADO, INT_FIN_QUANTUM, INT_ERROR
} estado_runner_thread_t;

typedef struct script_control_block {
	char *source;
	int request_pointer;
	estado_runner_thread_t estado;
} SCB;

int agregar_nuevo_script(char*);

void *correr_planificador(void*);

#endif
