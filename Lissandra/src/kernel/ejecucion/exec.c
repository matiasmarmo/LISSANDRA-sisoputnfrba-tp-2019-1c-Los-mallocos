#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <commons/string.h>

#include "../../commons/comunicacion/protocol.h"
#include "../../commons/lissandra-threads.h"
#include "../../commons/parser.h"
#include "../pool-memorias/manager-memorias.h"
#include "../kernel-config.h"
#include "planificador.h"
#include "exec.h"

void liberar_requests(char **requests) {
	char *request;
	int counter = 0;
	while ((request = requests[counter]) != NULL) {
		free(requests[counter]);
		counter++;
	}
	free(requests);
}

void ejecutar_add_request(uint8_t *request_buffer, SCB *scb) {
	struct add_request *add_request = (struct add_request*) request_buffer;
	if(agregar_memoria_a_criterio(add_request->n_memoria, add_request->criterio) < 0) {
		scb->estado = ERROR_SCRIPT;
	}
}

void ejecutar_request(char **request, SCB *scb) {
	// TODO: loguear representación en string de la respuesta a un archivo con resultados
	// y loguear errores.
	int tamanio_buffers = get_max_msg_size();
	uint8_t buffer_request[tamanio_buffers];
	uint8_t buffer_respuesta[tamanio_buffers];
	string_trim(request);
	if(*request == NULL) {
		scb->estado = ERROR_MISC;
		return;
	}
	if (parser(*request, buffer_request, tamanio_buffers) < 0) {
		scb->estado = ERROR_SCRIPT;
		return;
	}
	switch (get_msg_id(buffer_request)) {
	case ADD_REQUEST_ID:
		ejecutar_add_request(buffer_request, scb);
		break;
	case JOURNAL_REQUEST_ID:
		if (realizar_journal_a_todos_los_criterios() < 0) {
			scb->estado = ERROR_SCRIPT;
		}
		break;
	default:
		if (enviar_request_a_memoria(buffer_request, buffer_respuesta,
				tamanio_buffers) < 0) {
			scb->estado = ERROR_SCRIPT;
		}
	}
	destroy(buffer_request);
	destroy(buffer_respuesta);
	usleep(get_retardo_ejecucion() * 1000);
}

void *ejecutar_script(void *entrada) {
	lissandra_thread_t *self = (lissandra_thread_t*) entrada;
	SCB *scb = self->entrada;
	scb->estado = RUNNING;
	int quantum = get_quantum();
	int counter = 0;
	char **requests = string_split(scb->source, "\n");
	while (counter < quantum && requests[scb->request_pointer] != NULL) {
		ejecutar_request(&requests[scb->request_pointer], scb);
		if (scb->estado != RUNNING) {
			break;
		}
		counter++;
		scb->request_pointer++;
	}
	if (requests[scb->request_pointer] == NULL) {
		scb->estado = SCRIPT_FINALIZADO;
	} else if (counter == quantum) {
		scb->estado = INT_FIN_QUANTUM;
	}
	liberar_requests(requests);
	l_thread_indicar_finalizacion(self);
	pthread_exit(NULL);
}
