#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "../commons/comunicacion/protocol.h"
#include "../commons/comunicacion/sockets.h"
#include "../commons/consola/consola.h"
#include "../commons/lissandra-threads.h"
#include "../commons/consola/consola.h"
#include "../commons/comunicacion/protocol-utils.h"
#include "memoria-principal/memoria-handler.h"
#include "requests/memoria-insert-request-handler.h"
#include "requests/memoria-select-request-handler.h"
#include "requests/memoria-other-requests-handler.h"
#include "memoria-server.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-main.h"

t_list *lista_clientes;

cliente_t* construir_cliente(int valor) {
	cliente_t *cliente = malloc(sizeof(cliente_t));
	if (cliente == NULL) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Construcción de cliente fallida, error en malloc");
		return NULL;
	}
	cliente->cliente_valor = valor;
	return cliente;
}

void inicializar_clientes() {
	int valor_inicial = -1;
	cliente_t *cliente1 = construir_cliente(valor_inicial);
	cliente_t *cliente2 = construir_cliente(valor_inicial);
	lista_clientes = list_create();
	list_add(lista_clientes, cliente1);
	list_add(lista_clientes, cliente2);
}

void destruir_cliente( cliente_t *cliente) {
	free(cliente);
}

void destruir_clientes() {
	void _destruir_elemento(void *elemento) {
		destruir_cliente((cliente_t*) elemento);
	}

	list_destroy_and_destroy_elements(lista_clientes, &_destruir_elemento);
}

void sleep_acceso_memoria(bool should_sleep) {
	if(should_sleep) {
		usleep(get_retardo_memoria_principal() * 1000);
	}
}

int ejecutar_request(void *request, void *respuesta, bool should_sleep) {
	int resultado = 0;
	switch (get_msg_id(request)) {
	case SELECT_REQUEST_ID:
		bloquear_memoria();
		resultado = _manejar_select(*((struct select_request *) request),
				respuesta, should_sleep);
		desbloquear_memoria();
		sleep_acceso_memoria(should_sleep);
		return resultado;
	case INSERT_REQUEST_ID:
		bloquear_memoria();
		resultado = _manejar_insert(*((struct insert_request *) request),
				respuesta);
		desbloquear_memoria();
		sleep_acceso_memoria(should_sleep);
		return resultado;
	case CREATE_REQUEST_ID:
		return  _manejar_create(*((struct create_request *) request), respuesta, should_sleep);
	case DESCRIBE_REQUEST_ID:
		return _manejar_describe(*((struct describe_request *) request),
				respuesta, should_sleep);
	case GOSSIP_ID:
		return _manejar_gossip(*((struct gossip *) request), respuesta);
	case DROP_REQUEST_ID:
		return _manejar_drop(*((struct drop_request *) request),
				respuesta, should_sleep);
	case JOURNAL_REQUEST_ID:
		bloquear_memoria();
		resultado = realizar_journal();
		init_journal_response(resultado, respuesta);
		desbloquear_memoria();
		sleep_acceso_memoria(should_sleep);
		return resultado;
	default:
		return init_error_msg(0, "Comando no soportado", respuesta);
	}
}

void manejarCliente(cliente_t *cliente) {
	int res;
	int tamanio_buffers = get_max_msg_size();
	uint8_t buffer[tamanio_buffers], respuesta[tamanio_buffers];
	memset(buffer, 0, tamanio_buffers);
	memset(respuesta, 0, tamanio_buffers);
	if ((res = recv_msg(cliente->cliente_valor, buffer, tamanio_buffers)) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al recibir el mensaje del cliente, pudo haberse desconectado");
		if (res == SOCKET_ERROR || res == CONN_CLOSED) {
			close(cliente->cliente_valor);
			cliente->cliente_valor = -1;
		}
		return;
	}

	if (ejecutar_request(buffer, respuesta, true) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al ejecutar request");
		destroy(buffer);
		return;
	}

	destroy(buffer);
	if ((res = send_msg(cliente->cliente_valor, respuesta)) < 0) { // intento reconectarme
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al enviar el mensaje al cliente, pudo haberse desconectado");
		if (res == SOCKET_ERROR || res == CONN_CLOSED) {
			close(cliente->cliente_valor);
			cliente->cliente_valor = -1;
		}
	}
	destroy(respuesta);
}

bool cliente_esta_desconectado(void *cliente) {
	return ((cliente_t*) cliente)->cliente_valor == -1;
}

int aceptar_cliente(int servidor) {
	int cliente;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;

	if ((cliente = accept(servidor, (struct sockaddr*) &their_addr, &addr_size))
			== -1) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al aceptar la conexion del cliente");
		return -1;
	}
	return cliente;
}

void manejar_consola_memoria(char* linea, void* request) {
	if (get_msg_id(request) == EXIT_REQUEST_ID) {
		finalizar_memoria();
		return;
	}
	int tamanio_buffers = get_max_msg_size();
	uint8_t respuesta[tamanio_buffers];
	memset(respuesta, 0, tamanio_buffers);
	if (ejecutar_request(request, respuesta, false) < 0) {
		imprimir("Error al procesar el request\n");
		return;
	}
	mostrar(respuesta);
	destroy(request);
	destroy(respuesta);
}

void* correr_servidor_memoria(void* entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	char puerto[6];
	sprintf(puerto, "%d", get_puerto_escucha_mem());
	int servidor = create_socket_server(puerto, 10);
	if (servidor < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, true,
				"Fallo al crear el servidor");
		finalizar_memoria();
		pthread_exit(NULL);
	}

	iniciar_consola(&manejar_consola_memoria);
	inicializar_clientes();

	fd_set descriptores, copia;
	FD_ZERO(&descriptores);
	FD_SET(servidor, &descriptores);
	FD_SET(STDIN_FILENO, &descriptores);

	int i;
	int select_ret;
	int mayor_file_descriptor = servidor;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 250000000;

	while (!l_thread_debe_finalizar(l_thread)) {
		copia = descriptores;
		select_ret = pselect(mayor_file_descriptor + 1, &copia, NULL, NULL, &ts,
		NULL);
		if (select_ret == -1) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Fallo en la ejecucion del select del servidor %d", errno);
			break;
		} else if (select_ret > 0) {
			if (FD_ISSET(servidor, &copia)) {
				int client_socket = aceptar_cliente(servidor);
				if (client_socket > 0) {
					if (client_socket > mayor_file_descriptor) {
						mayor_file_descriptor = client_socket;
					}
					cliente_t *nuevo_cliente = malloc(sizeof(cliente_t));
					if (nuevo_cliente == NULL) {
						memoria_log_to_level(LOG_LEVEL_ERROR, false,
								"Error pedir nuevo cliente. Erro de malloc");
						close(client_socket);
					} else {
						nuevo_cliente->cliente_valor = client_socket;
						list_add(lista_clientes, nuevo_cliente);
						FD_SET(nuevo_cliente->cliente_valor, &descriptores);
					}
				}
			}
			if (FD_ISSET(STDIN_FILENO, &copia)) {
				leer_siguiente_caracter();
			}
			for (i = 0; i < list_size(lista_clientes); i++) {
				cliente_t *cliente = list_get(lista_clientes, i);
				int socket = cliente->cliente_valor;
				if (cliente->cliente_valor
						!= -1 && FD_ISSET(cliente->cliente_valor, &copia)) {
					manejarCliente(cliente);
					if(cliente_esta_desconectado(cliente)) {
						FD_CLR(socket, &descriptores);
					}
				}
			}
			list_remove_and_destroy_by_condition(lista_clientes, &cliente_esta_desconectado, free);
		}
	}
	destruir_clientes();
	cerrar_consola();
	close(servidor);
	l_thread_indicar_finalizacion(l_thread);
	pthread_exit(NULL);
}
