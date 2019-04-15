#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/string.h>

#include "../commons/comunicacion/sockets.h"
#include "../commons/comunicacion/protocol.h"
#include "../commons/comunicacion/protocol-utils.h"
#include "kernel-config.h"

typedef struct memoria {
	uint16_t id_memoria;
	char ip_memoria[16];
	char puerto_memoria[8];
	pthread_mutex_t mutex;
	int socket_fd;
	bool conectada;
} memoria_t;

uint16_t proximo_id_memoria = 0;

pthread_mutex_t memorias_mutex = PTHREAD_MUTEX_INITIALIZER;

t_list *pool_memorias;
t_list *criterio_sc;
t_list *criterio_shc;
t_list *criterio_ec;

void inicializar_listas() {
	pool_memorias = list_create();
	criterio_sc = list_create();
	criterio_shc = list_create();
	criterio_ec = list_create();
}

void destruir_memoria(memoria_t *memoria) {
	if (memoria->conectada) {
		close(memoria->socket_fd);
	}
	free(memoria);
}

void destruir_lista_y_memorias(t_list *lista) {
	void _destruir_elemento(void *elemento) {
		destruir_memoria((memoria_t*) elemento);
	}

	list_destroy_and_destroy_elements(lista, &_destruir_elemento);
}

void destruir_listas() {
	// El pool de memorias contiene todas las memorias, por lo que
	// destruimos todas las memorias ahí y en los criterios
	// solo necesitamos destruir las listas.
	destruir_lista_y_memorias(pool_memorias);
	free(criterio_sc);
	free(criterio_shc);
	free(criterio_ec);
}

memoria_t *construir_memoria(char *ip_memoria, uint16_t puerto_memoria) {
	memoria_t *memoria = malloc(sizeof(memoria_t));
	if (memoria == NULL) {
		return NULL;
	}
	if (pthread_mutex_init(&memoria->mutex, NULL) != 0) {
		free(memoria);
		return NULL;
	}
	memoria->id_memoria = proximo_id_memoria++;
	memset(memoria->ip_memoria, 0, 16);
	memset(memoria->puerto_memoria, 0, 8);
	strcpy(memoria->ip_memoria, ip_memoria);
	sprintf(memoria->puerto_memoria, "%d", puerto_memoria);
	memoria->socket_fd = -1;
	memoria->conectada = false;
	return memoria;
}

int conectar_memoria(memoria_t* memoria) {
	memoria->socket_fd = create_socket_client(memoria->ip_memoria,
			memoria->puerto_memoria);
	if (memoria->socket_fd < 0) {
		return -1;
	}
	memoria->conectada = true;
	return 0;
}

bool memoria_ya_conocida(memoria_t* memoria) {

	bool _igual_a_nueva_memoria(void *elemento) {
		memoria_t *memoria_lista = (memoria_t*) elemento;
		return strcmp(memoria->ip_memoria, memoria_lista->ip_memoria) == 0
				&& strcmp(memoria->puerto_memoria,
						memoria_lista->puerto_memoria) == 0;
	}

	return list_any_satisfy(pool_memorias, &_igual_a_nueva_memoria);
}

void agregar_nuevas_memorias(void *buffer) {
	struct gossip_response *mensaje = (struct gossip_response*) buffer;
	memoria_t *nueva_memoria;
	char ip_string[16] = { 0 };
	for (int i = 0; i < mensaje->ips_memorias_len; i++) {
		uint32_a_ipv4_string(mensaje->ips_memorias[i], ip_string, 16);
		nueva_memoria = construir_memoria(ip_string,
				mensaje->puertos_memorias[i]);
		if (nueva_memoria != NULL && !memoria_ya_conocida(nueva_memoria)) {
			// Si falló la construcción de una memoria o
			// la misma ya se encuentra en el pool,
			// no la agregamos al pool y continuamos con el resto.
			list_add(pool_memorias, nueva_memoria);
		}
		memset(ip_string, 0, 16);
	}
}

int obtener_memorias_del_pool(memoria_t *memoria_fuente) {
	int tamanio_maximo_respuesta = get_max_msg_size();
	uint8_t buffer_local[tamanio_maximo_respuesta];
	if (!memoria_fuente->conectada
			|| (pthread_mutex_lock(&memoria_fuente->mutex) != 0)) {
		return -1;
	}
	if (send_gossip(memoria_fuente->socket_fd) < 0) {
		return -1;
	}
	if (recv_msg(memoria_fuente->socket_fd, buffer_local,
			tamanio_maximo_respuesta) < 0) {
		return -1;
	}
	if (get_msg_id(buffer_local) != GOSSIP_RESPONSE_ID) {
		destroy(buffer_local);
		return -1;
	}
	agregar_nuevas_memorias(buffer_local);
	destroy(buffer_local);
	pthread_mutex_unlock(&memoria_fuente->mutex);
	return 0;
}

int actualizar_memorias() {
	int error;
	memoria_t *memoria_fuente;
	if((error = pthread_mutex_lock(&memorias_mutex)) != 0) {
		return -error;
	}
	for(int i = 0; i < list_size(pool_memorias); i++) {
		memoria_fuente = (memoria_t*) list_get(pool_memorias, i);
		if(obtener_memorias_del_pool(memoria_fuente) == 0) {
			// Memorias actualizadas
			pthread_mutex_unlock(&memorias_mutex);
			return 0;
		}
	}
	// No se pudo actualizar el pool consultando a
	// alguna de las memorias ya conocidas
	pthread_mutex_unlock(&memorias_mutex);
	return -1;
}

int inicializar_memorias() {
	int error;
	if ((error = pthread_mutex_lock(&memorias_mutex)) != 0) {
		return -error;
	}
	memoria_t *memoria_principal = construir_memoria(get_ip_memoria(),
			get_puerto_memoria());
	if (memoria_principal == NULL) {
		pthread_mutex_unlock(&memorias_mutex);
		return -1;
	}
	inicializar_listas();
	list_add(pool_memorias, memoria_principal);
	if (conectar_memoria(memoria_principal) < 0) {
		// destruir_listas() va a destruir la memoria principal,
		// la cual se encuentra dentros, por lo que no necesitamos
		// hacerlo acá a mano
		pthread_mutex_unlock(&memorias_mutex);
		proximo_id_memoria = 0;
		destruir_listas();
		return -1;
	}
	if (obtener_memorias_del_pool(memoria_principal) < 0) {
		pthread_mutex_unlock(&memorias_mutex);
		proximo_id_memoria = 0;
		destruir_listas();
		return -1;
	}
	pthread_mutex_unlock(&memorias_mutex);
	return 0;
}

int destruir_memorias() {
	int error;
	if ((error = pthread_mutex_lock(&memorias_mutex)) != 0) {
		return -error;
	}
	proximo_id_memoria = 0;
	destruir_listas();
	pthread_mutex_unlock(&memorias_mutex);
	return 0;
}


