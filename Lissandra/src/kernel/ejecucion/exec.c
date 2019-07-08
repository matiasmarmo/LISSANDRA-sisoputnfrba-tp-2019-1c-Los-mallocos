#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <commons/string.h>
#include <commons/log.h>

#include "../../commons/comunicacion/protocol.h"
#include "../../commons/consola/consola.h"
#include "../../commons/lissandra-threads.h"
#include "../../commons/parser.h"
#include "../pool-memorias/manager-memorias.h"
#include "../pool-memorias/metadata-tablas.h"
#include "../kernel-config.h"
#include "../metricas.h"
#include "../kernel-logger.h"
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

int ejecutar_add_request(uint8_t *request_buffer, uint8_t *buffer_respuesta,
		SCB *scb) {
	struct add_request *add_request = (struct add_request*) request_buffer;
	struct add_response *add_response = (struct add_response*) buffer_respuesta;
	bool fallo = false;
	if (agregar_memoria_a_criterio(add_request->n_memoria,
			add_request->criterio, scb->es_request_unitario) < 0) {
		scb->estado = ERROR_SCRIPT;
		fallo = true;
	}
	init_add_response(fallo, add_request->n_memoria, add_request->criterio,
			add_response);
	return 0;
}

int ejecutar_journal_request(uint8_t *buffer_respuesta, SCB *scb) {
	struct journal_response *journal_response =
			(struct journal_response*) buffer_respuesta;
	bool fallo;
	if (realizar_journal_a_todos_los_criterios() < 0) {
		scb->estado = ERROR_SCRIPT;
		fallo = true;
	}
	init_journal_response(fallo, journal_response);
	return 0;
}

int ejecutar_metrics_request(uint8_t *buffer_respuesta, SCB *scb) {
	char *metricas = obtener_metricas();
	struct metrics_response response;
	if (metricas == NULL
			|| init_metrics_response(false, metricas, &response) < 0) {
		scb->estado = ERROR_MISC;
		response.id = METRICS_RESPONSE_ID;
		response.fallo = true;
		response.resultado = NULL;
	}
	memcpy(buffer_respuesta, &response, sizeof(struct metrics_response));
	free(metricas);
	return 0;
}

int ejecutar_run_request(uint8_t *buffer_request, uint8_t *buffer_respuesta) {
	struct run_request *run_request = (struct run_request*) buffer_request;
	struct run_response *run_response = (struct run_response*) buffer_respuesta;
	if (agregar_nuevo_script(false, run_request->path) < 0
			|| init_run_response(false, run_request->path, run_response) < 0) {
		run_response->id = RUN_RESPONSE_ID;
		run_response->fallo = true;
		run_response->path = NULL;
	}
	return 0;
}

void ejecutar_request(char **request, SCB *scb) {
	// TODO: loguear representación en string de la respuesta a un archivo con resultados
	// y loguear errores.
	int ret;
	int tamanio_buffers = get_max_msg_size();
	uint8_t buffer_request[tamanio_buffers];
	uint8_t buffer_respuesta[tamanio_buffers];
	string_trim(request);
	if (*request == NULL) {
		scb->estado = ERROR_MISC;
		return;
	}
	if (parser(*request, buffer_request, tamanio_buffers) < 0) {
		scb->estado = ERROR_SCRIPT;
		kernel_log_to_level(LOG_LEVEL_INFO, scb->es_request_unitario, "Error de parseo: %s", *request);
		return;
	}
	if(!scb->es_request_unitario) {
		kernel_log_to_level(LOG_LEVEL_INFO, false, "Ejecutando: [%s]:[%s]", scb->name, *request);
	}
	switch (get_msg_id(buffer_request)) {
	case ADD_REQUEST_ID:
		ret = ejecutar_add_request(buffer_request, buffer_respuesta, scb);
		break;
	case JOURNAL_REQUEST_ID:
		ret = ejecutar_journal_request(buffer_respuesta, scb);
		break;
	case METRICS_REQUEST_ID:
		ret = ejecutar_metrics_request(buffer_respuesta, scb);
		break;
	case RUN_REQUEST_ID:
		ret = ejecutar_run_request(buffer_request, buffer_respuesta);
		break;
	case EXIT_REQUEST_ID:
		// Ignoramos la aparición de un exit en un script.
		// En caso de que el EXIT se ingrese por consola,
		// el módulo de consola del kernel finalizará
		// la ejecución del proceso y la linea no llegará
		// a este punto.
		break;
	default:
		if ((ret = enviar_request_a_memoria(buffer_request, buffer_respuesta,
				tamanio_buffers, scb->es_request_unitario)) < 0) {
			kernel_log_to_level(LOG_LEVEL_ERROR, scb->es_request_unitario,
					"El request '%s' no pudo ser enviado.", *request);
			scb->estado = ERROR_SCRIPT;
		}
	}
	destroy(buffer_request);
	if(ret == 0) {
		if(scb->es_request_unitario) {
			mostrar_async(buffer_respuesta);	
		}
		if(get_msg_id(buffer_respuesta) == GLOBAL_DESCRIBE_RESPONSE_ID) {
			cargar_tablas(*((struct global_describe_response*) buffer_respuesta));
		}
		destroy(buffer_respuesta);
	}
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
