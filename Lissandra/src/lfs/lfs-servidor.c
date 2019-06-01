#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
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
#include "lfs-logger.h"
#include "lfs-config.h"
#include "lfs-main.h"
#include "lissandra/lissandra.h"

void *manejar_cliente(void* entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	int cliente = *((int*) l_thread->entrada);
	fd_set descriptores, copia;
	FD_ZERO(&descriptores);
	FD_SET(cliente, &descriptores);
	int select_ret, error;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 250000000;
	int tamanio_buffers = get_max_msg_size();
	uint8_t buffer[tamanio_buffers], respuesta[tamanio_buffers];

	while (!l_thread_debe_finalizar(l_thread)) {
		copia = descriptores;
		select_ret = pselect(cliente + 1, &copia, NULL, NULL, &ts, NULL);
		if (select_ret == -1) {
			lfs_log_to_level(LOG_LEVEL_TRACE, false,
					"Fallo en la ejecucion del select del cliente");
			break;
		} else if (select_ret > 0) {
			if ((error = recv_msg(cliente, buffer, tamanio_buffers) < 0)) {
				lfs_log_to_level(LOG_LEVEL_TRACE, false,
						"Fallo al recibir el mensaje del cliente");
				if (error == SOCKET_ERROR || error == CONN_CLOSED) {
					break;
				} else {
					continue;
				}
			}

			if(manejar_request(buffer, respuesta) == -1){
				break;
			}

			destroy(buffer);
			if ((error = send_msg(cliente, respuesta) < 0)) {
				lfs_log_to_level(LOG_LEVEL_TRACE, false,
						"Fallo al enviar el mensaje al cliente");
				if (error == SOCKET_ERROR || error == CONN_CLOSED) {
					break;
				} else {
					continue;
				}
			}
		}
		memset(buffer, 0, tamanio_buffers);
		memset(respuesta, 0, tamanio_buffers);
	}

	close(cliente);
	free(l_thread->entrada);
	l_thread_indicar_finalizacion(l_thread);
	pthread_exit(NULL);
}

void crear_hilo_cliente(t_list *lista_clientes, int servidor) {
	int cliente;
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof their_addr;
	int *nuevo_cliente;
	lissandra_thread_t *nuevo_hilo;

	if ((cliente = accept(servidor, (struct sockaddr*) &their_addr, &addr_size))
			== -1) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al aceptar la conexion del cliente");
		return;
	}

	nuevo_cliente = malloc(sizeof(int));
	nuevo_hilo = malloc(sizeof(lissandra_thread_t));

	if (nuevo_cliente == NULL || nuevo_hilo == NULL) {
		free(nuevo_cliente);
		free(nuevo_hilo);
		lfs_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al alojar al cliente, error en malloc");
		close(cliente);
		return;
	}

	*nuevo_cliente = cliente;

	if (l_thread_create(nuevo_hilo, &manejar_cliente, nuevo_cliente) < 0) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al crear el hilo del Cliente: %d", cliente);
		free(nuevo_cliente);
		free(nuevo_hilo);
		send_error_msg(INTERNAL_SERVER_ERROR,
				"Error al inicializar estructuras", cliente);
		close(cliente);
		return;
	}

	if(send_lfs_handshake(get_tamanio_value(), cliente) < 0) {
		l_thread_solicitar_finalizacion(nuevo_hilo);
		l_thread_join(nuevo_hilo, NULL);
		free(nuevo_cliente);
		free(nuevo_hilo);
		close(cliente);
		return;
	}

	list_add(lista_clientes, nuevo_hilo);
}

bool termino_cliente(void *elemento) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) elemento;
	return (bool) l_thread_finalizo(l_thread);
}

void finalizar_hilo(void* elemento) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) elemento;
	l_thread_solicitar_finalizacion(l_thread);
	l_thread_join(l_thread, NULL);
	free(elemento);
}

void manejar_consola(char* linea, void* request) {
	void* respuesta[get_max_msg_size()];
	int res;
	if (get_msg_id(request) == EXIT_REQUEST_ID) {
		finalizar_lfs();
		return;
	}

	res = manejar_request(request, respuesta);
	if(res != -1){
		mostrar(respuesta);
		destroy(respuesta);
	}

	destroy(request);
}

void* correr_servidor(void* entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	// Usamos 6 caracteres porque el puerto llega hasta 65535 (o algo asi)
	// 5 caracters + el \0
	char puerto[6];
	sprintf(puerto, "%d", get_puerto_escucha());
	int servidor = create_socket_server(puerto, 10);

	if (servidor < 0) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false, "Fallo al crear el servidor");
		pthread_exit(NULL);
	}

	iniciar_consola(&manejar_consola);

	fd_set descriptores, copia;
	FD_ZERO(&descriptores);
	FD_SET(servidor, &descriptores);
	FD_SET(STDIN_FILENO, &descriptores);

	int select_ret;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 250000000;
	t_list* hilos_clientes = list_create();

	while (!l_thread_debe_finalizar(l_thread)) {
		copia = descriptores;
		select_ret = pselect(servidor + 1, &copia, NULL, NULL, &ts, NULL);
		if (select_ret == -1) {
			lfs_log_to_level(LOG_LEVEL_TRACE, false,
					"Fallo en la ejecucion del select del servidor");
			break;
		} else if (select_ret > 0) {
			if (FD_ISSET(servidor, &copia)) {
				crear_hilo_cliente(hilos_clientes, servidor);
			}
			if (FD_ISSET(STDIN_FILENO, &copia)) {
				leer_siguiente_caracter();
			}
		}
		list_remove_and_destroy_by_condition(hilos_clientes, &termino_cliente,
				&finalizar_hilo);
	}
	list_destroy_and_destroy_elements(hilos_clientes, &finalizar_hilo);
	cerrar_consola();
	close(servidor);
	l_thread_indicar_finalizacion(l_thread);
	pthread_exit(NULL);
}
