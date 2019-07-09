#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "../commons/comunicacion/protocol.h"
#include "../commons/comunicacion/protocol-utils.h"
#include "../commons/comunicacion/sockets.h"
#include "../commons/lissandra-threads.h"
#include "memoria-principal/memoria-handler.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

#include "memoria-gossip.h"

typedef struct seed {
	char ip[16];
	char puerto[6];
	int socket;
} seed_t;

pthread_mutex_t mutex_tabla_gossip = PTHREAD_MUTEX_INITIALIZER;
t_list* TABLA_DE_GOSSIP;
t_list* lista_seeds;

int agregar_esta_memoria_a_tabla_de_gossip();

void liberar_lista_strings(char **lista) {
	for(char **i = lista; *i != NULL; i++) {
		free(*i);
	}
	free(lista);
}

int agregar_seeds() {
	char **ips_seeds, **puertos_seeds, **ips_iterator, **puertos_iterator;
	ips_seeds = get_ip_seeds();
	if(ips_seeds == NULL) {
		memoria_log_to_level(LOG_LEVEL_ERROR, false,
			"Error al obtener los seeds");
		return -1;
	}
	puertos_seeds = get_puertos_seed();
	if(puertos_seeds == NULL) {
		memoria_log_to_level(LOG_LEVEL_ERROR, false,
			"Error al obtener los puertos");
		liberar_lista_strings(ips_seeds);
		return -1;
	}
	for(ips_iterator = ips_seeds, puertos_iterator = puertos_seeds; 
			*ips_iterator != NULL; ips_iterator++, puertos_iterator++) {
		char *ip = *ips_iterator;
		char *puerto = *puertos_iterator;
		seed_t *nuevo_seed = malloc(sizeof(seed_t));
		// Por ahora suponemos que el malloc no falla
		strcpy(nuevo_seed->ip, ip);
		strcpy(nuevo_seed->puerto, puerto);
		nuevo_seed->socket = -1;
		list_add(lista_seeds, nuevo_seed);
	}
	liberar_lista_strings(ips_seeds);
	liberar_lista_strings(puertos_seeds);
	return 0;
}

int inicializacion_tabla_gossip(){
	if(pthread_mutex_lock(&mutex_tabla_gossip) != 0) {
		return -1;
	}
	TABLA_DE_GOSSIP = list_create();
	lista_seeds = list_create();
	agregar_seeds();
	pthread_mutex_unlock(&mutex_tabla_gossip);
	agregar_esta_memoria_a_tabla_de_gossip();
	return 0;
}

void destruccion_tabla_gossip(){
	void _destroy_entrada_tabla_gossip(void *elemento) {
		free(elemento);
	}

	void _destroy_seed(void *e) {
		seed_t *seed = (seed_t*) e;
		if(seed->socket != -1) {
			close(seed->socket);
		}
		free(seed);
	}

	if(pthread_mutex_lock(&mutex_tabla_gossip) != 0) {
		return;
	}

	list_destroy_and_destroy_elements(TABLA_DE_GOSSIP,&_destroy_entrada_tabla_gossip);
	list_destroy_and_destroy_elements(lista_seeds, &_destroy_seed);

	pthread_mutex_unlock(&mutex_tabla_gossip);
}

int memoria_ya_existe(uint8_t numero_memoria) {
	bool _encontrada(void *memoria) {
		return ((memoria_gossip_t*) memoria)->numero_memoria == numero_memoria;
	}
	return list_find(TABLA_DE_GOSSIP, &_encontrada) != NULL;
}

int agregar_memoria_a_tabla_gossip(uint32_t ip, uint16_t puerto, uint8_t numero) {
	if(pthread_mutex_lock(&mutex_tabla_gossip) != 0) {
		return -1;
	}
	if(memoria_ya_existe(numero)) {
		pthread_mutex_unlock(&mutex_tabla_gossip);
		return 0;
	}
	memoria_gossip_t* nueva_memoria = malloc(sizeof(memoria_gossip_t));
	if(nueva_memoria == NULL) {
		pthread_mutex_unlock(&mutex_tabla_gossip);
		return -1;
	}
	nueva_memoria->numero_memoria = numero;
	nueva_memoria->ip_memoria = ip;
	nueva_memoria->puerto_memoria = puerto;
	list_add(TABLA_DE_GOSSIP, nueva_memoria);
	pthread_mutex_unlock(&mutex_tabla_gossip);
	return 0;
}

int agregar_esta_memoria_a_tabla_de_gossip(){
	char ip[16];
	uint32_t ip_uint32;
	if(get_ip_propia(ip, 16) < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Error al obtener mi ip");
		return -1;
	}
	ipv4_a_uint32(ip, &ip_uint32);
	return agregar_memoria_a_tabla_gossip(
		ip_uint32, get_puerto_escucha_mem(), get_numero_memoria());
}

int obtener_respuesta_gossip(void *respuesta){

	if(pthread_mutex_lock(&mutex_tabla_gossip) != 0) {
		return -1;
	}

	memoria_gossip_t *memoria_actual;
	uint32_t ips[list_size(TABLA_DE_GOSSIP)];
	uint16_t puertos[list_size(TABLA_DE_GOSSIP)];
	uint8_t numeros[list_size(TABLA_DE_GOSSIP)];
	for(int i = 0; i < list_size(TABLA_DE_GOSSIP); i++){
		memoria_actual = list_get(TABLA_DE_GOSSIP, i);
		ips[i] = memoria_actual->ip_memoria;
		puertos[i] = memoria_actual->puerto_memoria;
		numeros[i] = memoria_actual->numero_memoria;
	}
	int aux = init_gossip_response(list_size(TABLA_DE_GOSSIP), ips,
				list_size(TABLA_DE_GOSSIP), puertos,
				list_size(TABLA_DE_GOSSIP), numeros, respuesta);
	if(aux < 0) {
		memoria_log_to_level(LOG_LEVEL_TRACE, false,
				"Error al tratar de responder al gossip");
		pthread_mutex_unlock(&mutex_tabla_gossip);
		return -1;
	}
	pthread_mutex_unlock(&mutex_tabla_gossip);
	return 0;
}

void agregar_memorias_a_tabla(struct gossip_response* respuesta) {
	for(int i = 0; i < respuesta->ips_memorias_len; i++) {
		agregar_memoria_a_tabla_gossip(
			respuesta->ips_memorias[i],
			respuesta->puertos_memorias[i],
			respuesta->numeros_memorias[i]
		);
	}
}

int consultar_memorias_a_seed(seed_t *seed) {
	uint8_t respuesta[get_max_msg_size()];
	if(seed->socket == -1) {
		int socket_nuevo = create_socket_client(seed->ip, seed->puerto, FLAG_NON_BLOCK);
		if(socket_nuevo < 0
			|| wait_for_connection(socket_nuevo, 500) != 0) {
			return -1;
		}
		seed->socket = socket_nuevo;
	}
	socket_set_blocking(seed->socket);
	if(send_gossip(seed->socket) < 0) {
		// Se perdio la conexion
		close(seed->socket);
		seed->socket = -1;
		return -1;
	}
	if(recv_msg(seed->socket, respuesta, get_max_msg_size()) < 0) {
		close(seed->socket);
		seed->socket = -1;
		return -1;	
	}
	agregar_memorias_a_tabla((struct gossip_response*) respuesta);
	destroy(respuesta);
	return 0;
}

void realizar_gossip() {
	void _consultar(void *e) {
		consultar_memorias_a_seed((seed_t*) e);
	}

	list_iterate(lista_seeds, &_consultar);
}

void *realizar_gossip_threaded(void *entrada) {
	realizar_gossip();
	return NULL;
}
