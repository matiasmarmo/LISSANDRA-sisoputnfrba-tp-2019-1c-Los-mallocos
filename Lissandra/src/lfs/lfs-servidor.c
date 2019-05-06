#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <commons/collections/list.h>

#include "../commons/comunicacion/protocol.h"
#include "../commons/comunicacion/sockets.h"
#include "../commons/consola/consola.h"
#include "../commons/lissandra-threads.h"
#include "lfs-config.h"

void *manejar_cliente(void* entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	int cliente = *((int*) l_thread->entrada);
	fd_set descriptores, copia;
	FD_ZERO(&descriptores);
	FD_SET(cliente, &descriptores);
	int select_ret;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 250000000;
	int tamanio_buffers = get_max_msg_size();
	uint8_t buffer[tamanio_buffers], respuesta[tamanio_buffers];

	while (!l_thread_debe_finalizar(l_thread)) {
		copia = descriptores;
		select_ret = pselect(cliente + 1, &copia, NULL, NULL, &ts, NULL);
		if (select_ret == -1) {
			//logeamos el error
			break;
		} else if (select_ret > 0) {
			if (recv_msg(cliente, buffer, tamanio_buffers) < 0) {
				// TODO: recv_msg no falla solo si se cerró la conexion
				// Si recv_msg retorna SOCKET_ERROR o CONN_CLOSED, terminar cliente
				// Sino hacer continue;
				break;
			}
			//llamo a lissandra
			destroy(buffer);
			if (send_msg(cliente, respuesta) < 0) {
				// TODO: idem anterior
				break;
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
		// Error accept
		return;
	}

	nuevo_cliente = malloc(sizeof(int));
	nuevo_hilo = malloc(sizeof(lissandra_thread_t));

	if (nuevo_cliente == NULL || nuevo_hilo == NULL) {
		free(nuevo_cliente);
		free(nuevo_hilo);
		// Enviar mensaje de error al cliente
		close(cliente);
		return;
	}

	*nuevo_cliente = cliente;

	if (l_thread_create(nuevo_hilo, &manejar_cliente, nuevo_cliente) < 0) {
		// Fallo al crear el hilo
		free(nuevo_cliente);
		free(nuevo_hilo);
		// Enviar mensaje de error al cliente
		close(cliente);
		return;
	}
	list_add(lista_clientes, nuevo_hilo);
}

bool termino_cliente(void *elemento) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) elemento;
	return (bool) l_thread_finalizo(l_thread);
}

void fin_cliente(void* elemento) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) elemento;
	l_thread_solicitar_finalizacion(l_thread);
	l_thread_join(l_thread, NULL);
	free(elemento);
}

void* correr_servidor(void* entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	// Usamos 6 caracteres porque el puerto llega hasta 65535 (o algo asi)
	// 5 caracters + el \0
	char puerto[6];
	sprintf(puerto, "%d", get_puerto_escucha());
	int servidor = create_socket_server(puerto, 10);
	fd_set descriptores, copia;
	FD_ZERO(&descriptores);
	FD_SET(servidor, &descriptores);
	int select_ret;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 250000000;
	t_list* hilos_clientes = list_create();

	if (servidor < 0) {
		// logueo error
		// finalizar main
		pthread_exit(NULL);
	}

	while (!l_thread_debe_finalizar(l_thread)) {
		copia = descriptores;
		select_ret = pselect(servidor + 1, &copia, NULL, NULL, &ts, NULL);
		if (select_ret == -1) {
			//logeamos el error
			break;
		} else if (select_ret > 0) {
			if (FD_ISSET(servidor, &copia)) {
				crear_hilo_cliente(hilos_clientes, servidor);
			}
		}
		list_remove_and_destroy_by_condition(hilos_clientes, &termino_cliente,
				&fin_cliente);
	}
	list_destroy_and_destroy_elements(hilos_clientes, &fin_cliente);
	close(servidor);
	l_thread_indicar_finalizacion(l_thread);
	pthread_exit(NULL);
}
