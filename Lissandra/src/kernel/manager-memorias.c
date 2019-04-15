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
	list_destroy(criterio_sc);
	list_destroy(criterio_shc);
	list_destroy(criterio_ec);
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

int enviar_a_memoria(void *mensaje, memoria_t *memoria) {
	int error;
	if ((error = pthread_mutex_lock(&memoria->mutex)) != 0) {
		return -error;
	}
	if (!memoria->conectada && (conectar_memoria(memoria) < 0)) {
		pthread_mutex_unlock(&memoria->mutex);
		return -1;
	}
	if (send_msg(memoria->socket_fd, mensaje) < 0) {
		pthread_mutex_unlock(&memoria->mutex);
		return -1;
	}
	pthread_mutex_unlock(&memoria->mutex);
	return 0;
}

int recibir_mensaje_de_memoria(memoria_t *memoria, void *buffer,
		int tamanio_buffer) {
	int error;
	if ((error = pthread_mutex_lock(&memoria->mutex)) != 0) {
		return -error;
	}
	if(recv_msg(memoria->socket_fd, buffer, tamanio_buffer) < 0) {
		pthread_mutex_unlock(&memoria->mutex);
		return -1;
	}
	pthread_mutex_unlock(&memoria->mutex);
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
	int tamanio_buffer = get_max_msg_size();
	uint8_t buffer_local[tamanio_buffer];
	struct gossip mensaje;
	init_gossip(&mensaje);
	if (enviar_a_memoria(&mensaje, memoria_fuente) < 0) {
		return -1;
	}
	if (recibir_mensaje_de_memoria(memoria_fuente, buffer_local, tamanio_buffer) < 0) {
		return -1;
	}
	if (get_msg_id(buffer_local) != GOSSIP_RESPONSE_ID) {
		destroy(buffer_local);
		return -1;
	}
	agregar_nuevas_memorias(buffer_local);
	destroy(buffer_local);
	return 0;
}

int actualizar_memorias() {
	int error;
	memoria_t *memoria_fuente;
	if ((error = pthread_mutex_lock(&memorias_mutex)) != 0) {
		return -error;
	}
	for (int i = 0; i < list_size(pool_memorias); i++) {
		memoria_fuente = (memoria_t*) list_get(pool_memorias, i);
		if (obtener_memorias_del_pool(memoria_fuente) == 0) {
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
	if (obtener_memorias_del_pool(memoria_principal) < 0) {
		// destruir_listas() va a destruir la memoria principal,
		// la cual se encuentra dentros, por lo que no necesitamos
		// hacerlo acá a mano
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

memoria_t *buscar_memoria(t_list *lista, uint16_t id_memoria) {

	bool _memoria_encontrada(void *elemento) {
		memoria_t *memoria = (memoria_t*) elemento;
		return memoria->id_memoria == id_memoria;
	}
	return (memoria_t*) list_find(lista, &_memoria_encontrada);
}

memoria_t *buscar_memoria_en_pool(uint16_t id_memoria) {
	return buscar_memoria(pool_memorias, id_memoria);
}

int agregar_memoria_a_lista(t_list *lista, memoria_t *memoria) {
	if (buscar_memoria(lista, memoria->id_memoria) == NULL) {
		// La memoria no se encuentra ya en la lista
		list_add(lista, memoria);
		return 0;
	}
	return -1;
}

int realizar_journal_en_memoria(memoria_t *memoria) {
	int tamanio_buffer = get_max_msg_size();
	uint8_t buffer_local[tamanio_buffer];
	struct journal_request request;
	struct journal_response *respuesta;
	init_journal_request(&request);
	if (enviar_a_memoria(&request, memoria) < 0) {
		return -1;
	}
	if (recibir_mensaje_de_memoria(memoria, buffer_local, tamanio_buffer) < 0) {
		return -1;
	}
	if (get_msg_id(buffer_local) != JOURNAL_RESPONSE_ID) {
		return -1;
	}
	respuesta = (struct journal_response*) buffer_local;
	return !respuesta->fallo;
}

int realizar_journal_a_memorias_de_shc() {
	int resultado = 0;

	void _realizar_journal(void *elemento) {
		memoria_t *memoria = (memoria_t*) elemento;
		if (realizar_journal_en_memoria(memoria) < 0) {
			resultado = -1;
		}
	}

	list_iterate(criterio_shc, &_realizar_journal);
	return resultado;
}

int agregar_memoria_a_sc(uint16_t id_memoria) {
	int error;
	memoria_t *memoria;
	if ((error = pthread_mutex_lock(&memorias_mutex)) != 0) {
		return -error;
	}
	if (list_size(criterio_sc) > 0) {
		pthread_mutex_unlock(&memorias_mutex);
		return -1;
	}
	if ((memoria = buscar_memoria_en_pool(id_memoria)) == NULL) {
		pthread_mutex_unlock(&memorias_mutex);
		return -1;
	}
	agregar_memoria_a_lista(criterio_sc, memoria);
	pthread_mutex_unlock(&memorias_mutex);
	return 0;
}

int agregar_memoria_a_shc(uint16_t id_memoria) {
	int error;
	memoria_t *memoria;
	if ((error = pthread_mutex_lock(&memorias_mutex)) != 0) {
		return -error;
	}
	if (realizar_journal_a_memorias_de_shc() < 0) {
		pthread_mutex_unlock(&memorias_mutex);
		return -1;
	}
	if ((memoria = buscar_memoria_en_pool(id_memoria)) == NULL) {
		pthread_mutex_unlock(&memorias_mutex);
		return -1;
	}
	agregar_memoria_a_lista(criterio_shc, memoria);
	pthread_mutex_unlock(&memorias_mutex);
	return 0;
}

int agregar_memoria_a_ec(uint16_t id_memoria) {
	int error;
	memoria_t *memoria;
	if ((error = pthread_mutex_lock(&memorias_mutex)) != 0) {
		return -error;
	}
	if ((memoria = buscar_memoria_en_pool(id_memoria)) == NULL) {
		pthread_mutex_unlock(&memorias_mutex);
		return -1;
	}
	agregar_memoria_a_lista(criterio_ec, memoria);
	pthread_mutex_unlock(&memorias_mutex);
	return 0;
}

int agregar_memoria_a_criterio(uint16_t id_memoria, uint8_t criterio) {
	switch (criterio) {
	case SC:
		return agregar_memoria_a_sc(id_memoria);
	case SHC:
		return agregar_memoria_a_shc(id_memoria);
	case EC:
		return agregar_memoria_a_ec(id_memoria);
	}
	return -1;
}
