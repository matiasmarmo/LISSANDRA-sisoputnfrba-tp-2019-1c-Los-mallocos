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
	memset(buffer, 0, tamanio_buffers);
	memset(respuesta, 0, tamanio_buffers);
	if ((error = recv_msg(cliente, buffer, tamanio_buffers) < 0)) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al recibir el mensaje del cliente");
		if (error == SOCKET_ERROR || error == CONN_CLOSED) {
			sacarCliente(cliente, posicion);
			close(cliente);
		}
		return;
	}
	imprimir_async("Se envia respuesta al kernel");
	char *ips[2] = { "192.168.1.8", "192.168.1.8"};
	uint32_t ips_ints[2];
	for(int i = 0; i < 2; i++) {
		ipv4_a_uint32(ips[i], &(ips_ints[i]));
	}
	uint16_t puertos[2] = {1,2};
	uint8_t numeros[2] = {1,2};
	if(get_msg_id(buffer) == DESCRIBE_REQUEST_ID) {
		printf("Describe\n");
		send_global_describe_response(0, "TablaA;TablaB", 2, puertos, 2, numeros, 2, ips_ints, cliente);
	} else if (get_msg_id(buffer) == GOSSIP_ID) {
		printf("GOSSIP\n");
		send_gossip_response(2, ips_ints, 2, puertos, 2, numeros, cliente);
	} else {
		printf("Otro\n");
		send_drop_response(0, "Tabla", cliente);
	}

	return;
	// ejecutar_request(buffer, respuesta);
	destroy(buffer);
	if ((error = send_msg(cliente, respuesta) < 0)) { // intento reconectarme
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al enviar el mensaje al cliente");
		if (error == SOCKET_ERROR || error == CONN_CLOSED) {
			sacarCliente(cliente, posicion);
			close(cliente);
		}
		destroy(respuesta);
	}
	destroy(respuesta);
}

void agregarCliente(int nuevo_cliente) {
	if (cant_actual < 2){
		clientes[cant_actual] = nuevo_cliente;
		cant_actual++;
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
	int tamanio_buffers = get_max_msg_size();
	uint8_t respuesta[tamanio_buffers];
	memset(respuesta, 0, tamanio_buffers);
	// ejecutar_request(request, respuesta);
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
		memoria_log_to_level(LOG_LEVEL_TRACE, false, "Fallo al crear el servidor");
		pthread_exit(NULL);
	}

	iniciar_consola(&manejar_consola_memoria);

	fd_set descriptores, copia;
	FD_ZERO(&descriptores);
	FD_SET(servidor, &descriptores);
	FD_SET(STDIN_FILENO, &descriptores);

	int i;
	int nuevo_cliente;
	int select_ret;
	int mayor_file_descriptor = servidor;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 250000000;

	while (!l_thread_debe_finalizar(l_thread)) {
		copia = descriptores;
		select_ret = pselect(mayor_file_descriptor + 1, &copia, NULL, NULL, &ts, NULL);
		if (select_ret == -1) {
			memoria_log_to_level(LOG_LEVEL_TRACE, false,
					"Fallo en la ejecucion del select del servidor");
			break;
		} else if (select_ret > 0) {
			if (FD_ISSET(servidor, &copia)) {
				nuevo_cliente = aceptar_cliente(servidor);
				if(nuevo_cliente > 0) {
					agregarCliente(nuevo_cliente);
					if(nuevo_cliente > mayor_file_descriptor) {
						mayor_file_descriptor = nuevo_cliente;
					}
					FD_SET(clientes[cant_actual-1], &descriptores);
				}
			}
			if (FD_ISSET(STDIN_FILENO, &copia)) {
				leer_siguiente_caracter();
			}
			for(i=0; i<cant_actual ; i++) {
				if(clientes[i] != -1 && FD_ISSET(clientes[i], &copia)) {
					manejarCliente(clientes[i], &i);
				}
			}
		}
	}
	cerrar_consola();
	close(servidor);
	l_thread_indicar_finalizacion(l_thread);
	pthread_exit(NULL);
}
