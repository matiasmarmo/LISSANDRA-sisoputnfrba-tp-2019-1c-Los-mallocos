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
#include "memoria-server.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-main.h"

t_list *lista_clientes;

//int clientes[2]={-1, -1};
//int cant_actual=0;

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

void destruir_cliente(cliente_t *cliente) {
	free(cliente);
}

void destruir_clientes() {
	void _destruir_elemento(void *elemento) {
			destruir_cliente((cliente_t*) elemento);
		}

	list_destroy_and_destroy_elements(lista_clientes, &_destruir_elemento);
}

void sacarCliente(int cliente, int* posicion) {
	int valor_inicial = -1;
	cliente_t *clienteReseteado = construir_cliente(valor_inicial);
	if(posicion == 0) {
		list_add_in_index(lista_clientes, 0, list_get(lista_clientes, 1));
		list_add_in_index(lista_clientes, 1, clienteReseteado);
	} else {
		list_add_in_index(lista_clientes, 1, clienteReseteado);
	}
}

int manejarErrores(int retorno_protocol){
	if(retorno_protocol == BUFFER_TOO_SMALL) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
		    "Buffer demasiado pequeño");
				return -1;
	} if(retorno_protocol == ALLOC_ERROR) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
			"El valor de la tabla es NULL");
				return -1;
	}
	return 1;
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

	uint8_t info_decodificada[tamanio_buffers];
	memset(info_decodificada, 0, tamanio_buffers);

	int recibir;
	switch(get_msg_id(buffer)){
	case SELECT_REQUEST_ID:
		//select_request *request_select = malloc(sizeof(select_request));
		//struct select_request request_select;
		recibir = decode_select_request (buffer , info_decodificada , tamanio_buffers);
		if(manejarErrores(recibir) > 0) {
			_manejar_select(info_decodificada);
		}
		//free(request_select);
		break;
	case INSERT_REQUEST_ID:
		//insert_request *request_insert = malloc(sizeof(insert_request));
		//struct insert_request request_select;
		recibir = decode_insert_request (buffer , info_decodificada , tamanio_buffers);
		if(manejarErrores(recibir) > 0) {
			_manejar_insert(info_decodificada);
		}
		//free(request_insert);
		break;
	default:
		imprimir_async("Se recibió una solicitud que aun no se puede trabajar");
		break;
	}

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

	/*
		imprimir_async("Se envia respuesta al kernel");
		char *ips[2] = { "192.168.1.8", "192.168.1.8"};
		uint32_t ips_ints[2];
		for(int i = 0; i < 2; i++) {
			ipv4_a_uint32(ips[i], &(ips_ints[i]));
		}
		uint16_t puertos[2] = {1,2};
		uint8_t numeros[2] = {1,2};*/

   /*
	if(get_msg_id(buffer) == DESCRIBE_REQUEST_ID) {
		printf("Describe\n");
		//send_global_describe_response(0, "TablaA;TablaB", 2, puertos, 2, numeros, 2, ips_ints, cliente);
	} else if (get_msg_id(buffer) == GOSSIP_ID) {
		printf("GOSSIP\n");
		send_gossip_response(2, ips_ints, 2, puertos, 2, numeros, cliente);
	} else {
		printf("Otro\n");
		send_drop_response(0, "Tabla", cliente);
	}
	*/

	//return;
	// ejecutar_request(buffer, respuesta);
}

void agregarCliente(int valor_cliente) {
	cliente_t *clienteNuevo = construir_cliente(valor_cliente);
	cliente_t *primerCliente = list_get(lista_clientes, 0);
	cliente_t *segundoCliente = list_get(lista_clientes, 0);;
	if (primerCliente->cliente_valor == -1 || segundoCliente->cliente_valor == -1) {
		if(primerCliente->cliente_valor == -1){
			list_add_in_index(lista_clientes, 0, clienteNuevo);
		} else {
			list_add_in_index(lista_clientes, 1, clienteNuevo);
		}
	}
	else{
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
	cliente_t *cliente = malloc(sizeof(cliente_t));
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
					//list_add(lista_clientes,nuevo_cliente);
					agregarCliente(nuevo_cliente);
					if(nuevo_cliente > mayor_file_descriptor) {
						mayor_file_descriptor = nuevo_cliente;
					}
					cliente = list_get(lista_clientes, list_size(lista_clientes) - 1);
					FD_SET(cliente->cliente_valor, &descriptores);
					//FD_SET(clientes[cant_actual-1], &descriptores);
				}
			}
			if (FD_ISSET(STDIN_FILENO, &copia)) {
				leer_siguiente_caracter();
			}
			for(i=0; i < list_size(lista_clientes) ; i++) {
				//if(clientes[i] != -1 && FD_ISSET(clientes[i], &copia)) {
				cliente = list_get(lista_clientes, i);

				if(cliente->cliente_valor != -1 && FD_ISSET(cliente->cliente_valor, &copia)) {
					manejarCliente(cliente->cliente_valor, &i);
				}
			}
		}
	}
	free(cliente);
	cerrar_consola();
	close(servidor);
	l_thread_indicar_finalizacion(l_thread);
	pthread_exit(NULL);
}
