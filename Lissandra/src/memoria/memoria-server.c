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
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-main.h"

int clientes[2]={-1, -1};
int cant_actual=0;

void sacarCliente(int cliente, int* posicion) {
	if(posicion == 0) {
		clientes[0] = -1;
		if(clientes[1] != -1) {
			clientes[0] = clientes[1];
			clientes[1] = -1;
			posicion--;
		}
	} else {
		clientes[1] = -1;
	}
	cant_actual--;
}

void manejarCliente(int cliente, int* posicion){
	int error;
	int tamanio_buffers = get_max_msg_size();
	uint8_t buffer[tamanio_buffers], respuesta[tamanio_buffers];
	if ((error = recv_msg(cliente, buffer, tamanio_buffers) < 0)) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al recibir el mensaje del cliente");
		if (error == SOCKET_ERROR) {
			//break;
		} if (error == CONN_CLOSED) {
			sacarCliente(cliente, posicion);
		} else {
			//continue;
		}
	}
	destroy(buffer);
	if ((error = send_msg(cliente, respuesta) < 0)) { // intento reconectarme
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al enviar el mensaje al cliente");
		if (error == SOCKET_ERROR) {
			//break;
		} if (error == CONN_CLOSED) {
			//sacarCliente(cliente, posicion);
		} else {
			//continue;
		}
	}
	memset(buffer, 0, tamanio_buffers);
	close(cliente);
}

void agregarCliente(int* nuevo_cliente) {
	if (cant_actual < 2){
		clientes[cant_actual] = nuevo_cliente;
		cant_actual ++;
	} else {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Se intentó conectar un tercer cliente en la memoria");
	}
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
	}
	//llamar a lissandra(request)
	//manejar error (si tira error)
	destroy(request);
}

void* correr_servidor_memoria(void* entrada) {
	lissandra_thread_t *l_thread = (lissandra_thread_t*) entrada;
	char puerto[6];
	sprintf(puerto, "%d", get_puerto_escucha_mem());

	int servidor = create_socket_server(puerto, 10);
	if (servidor < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false, "Fallo al crear el servidor");
		pthread_exit(NULL);
	}

	//iniciar_consola(&manejar_consola);

	fd_set descriptores, copia;
	FD_ZERO(&descriptores);
	FD_SET(servidor, &descriptores);
	FD_SET(STDIN_FILENO, &descriptores);

	int i;
	int *nuevo_cliente;
	int select_ret;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 250000000;

	while (!l_thread_debe_finalizar(l_thread)) {
		copia = descriptores;
		select_ret = pselect(servidor + 1, &copia, NULL, NULL, &ts, NULL);
		if (select_ret == -1) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Fallo en la ejecucion del select del servidor");
			break;
		} else if (select_ret > 0) {
			if (FD_ISSET(servidor, &copia)) {
				*nuevo_cliente = aceptar_cliente(servidor);
				if(nuevo_cliente >= 0) {
					agregarCliente(nuevo_cliente);
					FD_SET(clientes[cant_actual-1], &descriptores);
				}
			}
			if (FD_ISSET(STDIN_FILENO, &copia)) {
				leer_siguiente_caracter();
				*nuevo_cliente = aceptar_cliente(servidor);
				if(nuevo_cliente >= 0) {
					agregarCliente(nuevo_cliente);
					FD_SET(clientes[cant_actual-1], &descriptores);
				}
			}
			for(i=0; i<cant_actual ; i++) {
				if(clientes[i] != -1) {
					manejarCliente(clientes[i], &i);
				}
			}
			free(nuevo_cliente);
		}
	}
	cerrar_consola();
	close(servidor);
	l_thread_indicar_finalizacion(l_thread);
	pthread_exit(NULL);
}
