#include <unistd.h>
#include <stdbool.h>

#include "../commons/comunicacion/protocol.h"
#include "../commons/consola/consola.h"
#include "../commons/lissandra-threads.h"
#include "ejecucion/planificador.h"
#include "kernel-main.h"

void manejar_nuevo_request(char *linea, void *request) {
	int msg_id = get_msg_id(request);
	if(msg_id == RUN_REQUEST_ID) {
		struct run_request *run_request = (struct run_request*) request;
		agregar_nuevo_script(false, run_request->path);
	} else if(msg_id == EXIT_REQUEST_ID){
		finalizar_kernel();
	} else {
		agregar_nuevo_script(true, linea);
	}
	destroy(request);
}

void *correr_consola(void *entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	iniciar_consola(&manejar_nuevo_request);
	while(!l_thread_debe_finalizar(l_thread)) {
		leer_siguiente_caracter();
		usleep(100);
	}
	cerrar_consola();
	pthread_exit(NULL);
}
