#include <unistd.h>
#include <stdbool.h>
#include <sys/select.h>

#include "../commons/comunicacion/protocol.h"
#include "../commons/consola/consola.h"
#include "../commons/lissandra-threads.h"
#include "ejecucion/planificador.h"
#include "kernel-main.h"

bool finalizacion_indicada = false;

void manejar_nuevo_request(char *linea, void *request) {
	int msg_id = get_msg_id(request);
	if(msg_id == RUN_REQUEST_ID) {
		struct run_request *run_request = (struct run_request*) request;
		agregar_nuevo_script(false, run_request->path);
	} else if(msg_id == EXIT_REQUEST_ID){
		finalizar_kernel();
		finalizacion_indicada = true;
	} else {
		agregar_nuevo_script(true, linea);
	}
	destroy(request);
}

void *correr_consola(void *entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	iniciar_consola(&manejar_nuevo_request);

	fd_set stdin_set, copy_set;
	struct timespec ts;
	int select_ret;
	FD_ZERO(&stdin_set);
	FD_SET(STDIN_FILENO, &stdin_set);
	ts.tv_sec = 0;
	ts.tv_nsec = 1000000;

	while(!l_thread_debe_finalizar(l_thread) && !finalizacion_indicada) {
		copy_set = stdin_set;
		select_ret = pselect(STDIN_FILENO + 1, &copy_set, NULL, NULL, &ts, NULL);
		if(select_ret > 0) {
			leer_siguiente_caracter();
		}
	}
	cerrar_consola();
	pthread_exit(NULL);
}
