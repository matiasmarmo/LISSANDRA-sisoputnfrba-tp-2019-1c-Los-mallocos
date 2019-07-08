#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/log.h>

#include "../../commons/comunicacion/sockets.h"
#include "../../commons/comunicacion/protocol.h"
#include "../../commons/comunicacion/protocol-utils.h"
#include "../../commons/consola/consola.h"
#include "../kernel-logger.h"
#include "../kernel-config.h"
#include "../metricas.h"
#include "metadata-tablas.h"
#include "manager-memorias.h"

pthread_rwlock_t memorias_rwlock = PTHREAD_RWLOCK_INITIALIZER;

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
	pthread_mutex_lock(&memoria->mutex);
	pthread_mutex_unlock(&memoria->mutex);
	pthread_mutex_destroy(&memoria->mutex);
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

memoria_t *construir_memoria(char *ip_memoria, uint16_t puerto_memoria,
		uint8_t numero_memoria) {
	memoria_t *memoria = malloc(sizeof(memoria_t));
	if (memoria == NULL) {
		kernel_log_to_level(LOG_LEVEL_TRACE, false,
				"Construcción de memoria_t fallida, error en malloc");
		return NULL;
	}
	memoria->id_memoria = numero_memoria;
	memset(memoria->ip_memoria, 0, 16);
	memset(memoria->puerto_memoria, 0, 8);
	strcpy(memoria->ip_memoria, ip_memoria);
	sprintf(memoria->puerto_memoria, "%d", puerto_memoria);
	memoria->socket_fd = -1;
	memoria->conectada = false;
	if (pthread_mutex_init(&memoria->mutex, NULL) != 0) {
		kernel_log_to_level(LOG_LEVEL_TRACE, false,
				"Construcción de memoria_t fallida, error al inicializar mutex");
		free(memoria);
		return NULL;
	}
	return memoria;
}

int conectar_memoria(memoria_t* memoria) {
	// Para conectarnos utilizamos un socket no bloqueante y lo esperamos
	// 500ms a que se conecte. Si no logra conectarse en ese tiempo,
	// wait_for_connection lo cierra y nos retorna distínto de 0.
	// Por el momento las lecturas y escrituras se realizan en modo
	// bloqueante, por lo que lo convertimos nuevamente a este estado.
	memoria->socket_fd = create_socket_client(memoria->ip_memoria,
			memoria->puerto_memoria, FLAG_NON_BLOCK);
	if (memoria->socket_fd < 0
			|| wait_for_connection(memoria->socket_fd, 500) != 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, false,
				"No pudo conectarse la memoria número %d", memoria->id_memoria);
		return -1;
	}
	if (socket_set_blocking(memoria->socket_fd) < 0) {
		kernel_log_to_level(LOG_LEVEL_DEBUG, false,
				"Error al convertir al socket de la memoria %d de bloqueante a no bloqueante",
				memoria->id_memoria);
		close(memoria->socket_fd);
		return -1;
	}
	memoria->conectada = true;
	return 0;
}

int realizar_journal_a_memorias_de_shc();

void remover_memoria_de_sus_criterios(memoria_t *memoria) {

	bool _memoria_encontrada(void *elemento) {
		return ((memoria_t*) elemento)->id_memoria == memoria->id_memoria;
	}

	list_remove_by_condition(criterio_sc, &_memoria_encontrada);
	if (list_remove_by_condition(criterio_shc, &_memoria_encontrada) != NULL) {
		// Si estaba en el criterio shc y se la removió, realizamos un journal
		// a las memorias del criterio
		realizar_journal_a_memorias_de_shc();
	}
	list_remove_by_condition(criterio_ec, &_memoria_encontrada);
}

int enviar_a_memoria(void *mensaje, memoria_t *memoria) {
	if (!memoria->conectada && (conectar_memoria(memoria) < 0)) {
		return -1;
	}
	if (send_msg(memoria->socket_fd, mensaje) < 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, true, "Memoria número %d caída.",
				memoria->id_memoria);
		memoria->conectada = false;
		close(memoria->socket_fd);
		remover_memoria_de_sus_criterios(memoria);
		return -1;
	}
	return 0;
}

int recibir_mensaje_de_memoria(memoria_t *memoria, void *buffer,
		int tamanio_buffer) {
	if (recv_msg(memoria->socket_fd, buffer, tamanio_buffer) < 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, true, "Memoria número %d caída.",
				memoria->id_memoria);
		memoria->conectada = false;
		close(memoria->socket_fd);
		remover_memoria_de_sus_criterios(memoria);
		return -1;
	}
	return 0;
}

int enviar_y_recibir_respuesta(void *mensaje, memoria_t *memoria,
		void *respuesta, int tamanio_respuesta) {
	if (tamanio_respuesta < get_max_msg_size()) {
		return -1;
	}
	if (enviar_a_memoria(mensaje, memoria) < 0) {
		return -1;
	}
	if (recibir_mensaje_de_memoria(memoria, respuesta, tamanio_respuesta) < 0) {
		return -1;
	}
	if (get_msg_id(respuesta) == MEMORY_FULL_ID) {
		kernel_log_to_level(LOG_LEVEL_INFO, false,
				"Memoria numero %d llena, reintentando", memoria->id_memoria);
	}
	// El journal request no hace falta destruirlo porque no tiene memoria dinámica
	struct journal_request journal_req;
	init_journal_request(&journal_req);
	while (get_msg_id(respuesta) == MEMORY_FULL_ID) {
		destroy(respuesta);
		if(enviar_a_memoria(&journal_req, memoria) < 0) {
			return -1;
		}
		usleep(100000);
		if (enviar_a_memoria(mensaje, memoria) < 0) {
			return -1;
		}
		if (recibir_mensaje_de_memoria(memoria, respuesta, tamanio_respuesta)
				< 0) {
			return -1;
		}
	}
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
		memset(ip_string, 0, 16);
		uint32_a_ipv4_string(mensaje->ips_memorias[i], ip_string, 16);
		nueva_memoria = construir_memoria(ip_string,
				mensaje->puertos_memorias[i], mensaje->numeros_memorias[i]);
		if (nueva_memoria == NULL) {
			kernel_log_to_level(LOG_LEVEL_WARNING, false,
					"No pudo relevarse la información de un memoria (%s, %d), omitiendo",
					ip_string, mensaje->puertos_memorias[i]);
			continue;
		}
		if (!memoria_ya_conocida(nueva_memoria)) {
			// Si falló la construcción de una memoria o
			// la misma ya se encuentra en el pool,
			// no la agregamos al pool y continuamos con el resto.
			list_add(pool_memorias, nueva_memoria);
		} else {
			destruir_memoria(nueva_memoria);
		}
	}
}

int obtener_memorias_del_pool(memoria_t *memoria_fuente) {
	int tamanio_buffer = get_max_msg_size();
	uint8_t buffer_local[tamanio_buffer];
	struct gossip mensaje;
	init_gossip(&mensaje);
	if (pthread_mutex_lock(&memoria_fuente->mutex) != 0) {
		return -1;
	}
	if (enviar_y_recibir_respuesta(&mensaje, memoria_fuente, buffer_local,
			tamanio_buffer) < 0) {
		pthread_mutex_unlock(&memoria_fuente->mutex);
		return -1;
	}
	pthread_mutex_unlock(&memoria_fuente->mutex);
	if (get_msg_id(buffer_local) != GOSSIP_RESPONSE_ID) {
		kernel_log_to_level(LOG_LEVEL_DEBUG, false,
				"Respuesta inesperada de memoria. Se esperaba %d (GOSSIP_RESPONSE), pero se recibio %d",
				GOSSIP_RESPONSE_ID, get_msg_id(buffer_local));
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
	if ((error = pthread_rwlock_wrlock(&memorias_rwlock)) != 0) {
		return -error;
	}
	for (int i = 0; i < list_size(pool_memorias); i++) {
		memoria_fuente = (memoria_t*) list_get(pool_memorias, i);
		if (obtener_memorias_del_pool(memoria_fuente) == 0) {
			// Memorias actualizadas
			pthread_rwlock_unlock(&memorias_rwlock);
			return 0;
		}
	}
	// No se pudo actualizar el pool consultando a
	// alguna de las memorias ya conocidas
	pthread_rwlock_unlock(&memorias_rwlock);
	kernel_log_to_level(LOG_LEVEL_WARNING, false, "Fallo al realizar gossip");
	return -1;
}

void *actualizar_memorias_threaded(void *entrada) {
	actualizar_memorias();
	return NULL;
}

int inicializar_memorias() {
	int error;
	if ((error = pthread_rwlock_wrlock(&memorias_rwlock)) != 0) {
		return -error;
	}
	memoria_t *memoria_principal = construir_memoria(get_ip_memoria(),
			get_puerto_memoria(), get_numero_memoria());
	if (memoria_principal == NULL) {
		pthread_rwlock_unlock(&memorias_rwlock);
		return -1;
	}
	inicializar_listas();
	list_add(pool_memorias, memoria_principal);
	if (obtener_memorias_del_pool(memoria_principal) < 0) {
		// destruir_listas() va a destruir la memoria principal,
		// la cual se encuentra dentros, por lo que no necesitamos
		// hacerlo acá a mano
		pthread_rwlock_unlock(&memorias_rwlock);
		destruir_listas();
		kernel_log_to_level(LOG_LEVEL_ERROR, false,
				"Fallo al realizar gossip inicial");
		return -1;
	}
	pthread_rwlock_unlock(&memorias_rwlock);
	inicializar_tablas();
	return 0;
}

int destruir_memorias() {
	int error;
	destruir_tablas();
	if ((error = pthread_rwlock_wrlock(&memorias_rwlock)) != 0) {
		return -error;
	}
	destruir_listas();
	pthread_rwlock_unlock(&memorias_rwlock);
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
	if (pthread_mutex_lock(&memoria->mutex) != 0) {
		return -1;
	}
	if (enviar_y_recibir_respuesta(&request, memoria, buffer_local,
			tamanio_buffer) < 0) {
		pthread_mutex_unlock(&memoria->mutex);
		return -1;
	}
	pthread_mutex_unlock(&memoria->mutex);
	if (get_msg_id(buffer_local) != JOURNAL_RESPONSE_ID) {
		kernel_log_to_level(LOG_LEVEL_DEBUG, false,
				"Respuesta inesperada de memoria. Se esperaba %d (JOURNAL_RESPONSE), pero se recibio %d",
				JOURNAL_RESPONSE_ID, get_msg_id(buffer_local));
		return -1;
	}
	respuesta = (struct journal_response*) buffer_local;
	return !respuesta->fallo;
}

int realizar_journal_a_lista(t_list *lista_memorias) {

	int resultado = 0;

	void _realizar_journal(void *elemento) {
		memoria_t *memoria = (memoria_t*) elemento;
		if (realizar_journal_en_memoria(memoria) < 0) {
			resultado = -1;
		}
	}

	list_iterate(lista_memorias, &_realizar_journal);
	return resultado;
}

int realizar_journal_a_memorias_de_shc() {
	return realizar_journal_a_lista(criterio_shc);
}

int realizar_journal_a_todos_los_criterios() {
	int resultado = realizar_journal_a_lista(criterio_sc);
	resultado = realizar_journal_a_lista(criterio_shc) < 0 ? -1 : resultado;
	resultado = realizar_journal_a_lista(criterio_ec) < 0 ? -1 : resultado;
	return resultado;
}

int agregar_memoria_a_sc(uint16_t id_memoria, int es_request_unitario) {
	int error;
	memoria_t *memoria;
	if ((error = pthread_rwlock_wrlock(&memorias_rwlock)) != 0) {
		return FALLO_BLOQUEO_SEMAFORO;
	}
	if (list_size(criterio_sc) > 0) {
		pthread_rwlock_unlock(&memorias_rwlock);
		int id_memoria_ya_asociada =
				((memoria_t*) list_get(criterio_sc, 0))->id_memoria;
		if (id_memoria_ya_asociada != id_memoria) {
			kernel_log_to_level(LOG_LEVEL_WARNING, es_request_unitario,
					"No se agregó la memoria %d al criterio SC, la memoria %d ya está asociada a el",
					id_memoria, id_memoria_ya_asociada);
		}
		return -1;
	}
	if ((memoria = buscar_memoria_en_pool(id_memoria)) == NULL) {
		pthread_rwlock_unlock(&memorias_rwlock);
		kernel_log_to_level(LOG_LEVEL_WARNING, es_request_unitario,
				"Memoria numero %d desconocida", id_memoria);
		return MEMORIA_DESCONOCIDA;
	}
	agregar_memoria_a_lista(criterio_sc, memoria);
	pthread_rwlock_unlock(&memorias_rwlock);
	return 0;
}

int agregar_memoria_a_shc(uint16_t id_memoria, int es_request_unitario) {
	int error;
	memoria_t *memoria;
	if ((error = pthread_rwlock_wrlock(&memorias_rwlock)) != 0) {
		return FALLO_BLOQUEO_SEMAFORO;
	}
	if (buscar_memoria(criterio_shc, id_memoria) != NULL) {
		// La memoria ya se encuentra en SHC
		pthread_rwlock_unlock(&memorias_rwlock);
		return 0;
	}
	if (realizar_journal_a_memorias_de_shc() < 0) {
		pthread_rwlock_unlock(&memorias_rwlock);
		return -1;
	}
	if ((memoria = buscar_memoria_en_pool(id_memoria)) == NULL) {
		pthread_rwlock_unlock(&memorias_rwlock);
		kernel_log_to_level(LOG_LEVEL_WARNING, es_request_unitario,
				"Memoria numero %d desconocida", id_memoria);
		return MEMORIA_DESCONOCIDA;
	}
	agregar_memoria_a_lista(criterio_shc, memoria);
	pthread_rwlock_unlock(&memorias_rwlock);
	return 0;
}

int agregar_memoria_a_ec(uint16_t id_memoria, int es_request_unitario) {
	int error;
	memoria_t *memoria;
	if ((error = pthread_rwlock_wrlock(&memorias_rwlock)) != 0) {
		return FALLO_BLOQUEO_SEMAFORO;
	}
	if ((memoria = buscar_memoria_en_pool(id_memoria)) == NULL) {
		pthread_rwlock_unlock(&memorias_rwlock);
		kernel_log_to_level(LOG_LEVEL_WARNING, es_request_unitario,
				"Memoria numero %d desconocida", id_memoria);
		return MEMORIA_DESCONOCIDA;
	}
	agregar_memoria_a_lista(criterio_ec, memoria);
	pthread_rwlock_unlock(&memorias_rwlock);
	return 0;
}

int agregar_memoria_a_criterio(uint16_t id_memoria, uint8_t criterio,
		int es_request_unitario) {
	int resultado;
	switch (criterio) {
	case SC:
		resultado = agregar_memoria_a_sc(id_memoria, es_request_unitario);
		break;
	case SHC:
		resultado = agregar_memoria_a_shc(id_memoria, es_request_unitario);
		break;
	case EC:
		resultado = agregar_memoria_a_ec(id_memoria, es_request_unitario);
		break;
	}
	if (resultado == MEMORIA_DESCONOCIDA) {
		kernel_log_to_level(LOG_LEVEL_WARNING, es_request_unitario,
				"Memoria número %d desconocida.", id_memoria);
	}
	return resultado;
}

memoria_t *memoria_random(t_list *lista_memorias) {
	if (list_size(lista_memorias) == 0) {
		return NULL;
	}
	return list_get(lista_memorias, rand() % list_size(lista_memorias));
}

memoria_t *memoria_conectada_random(t_list *lista_memorias) {

	bool _esta_conectada(void *elemento) {
		memoria_t *memoria = (memoria_t*) elemento;
		if (pthread_mutex_lock(&memoria->mutex) != 0) {
			return false;
		}
		bool resultado = ((memoria_t*) elemento)->conectada;
		pthread_mutex_unlock(&memoria->mutex);
		return resultado;
	}

	t_list *memorias_conectadas = list_filter(lista_memorias, &_esta_conectada);
	memoria_t *memoria_resultado = memoria_random(memorias_conectadas);
	list_destroy(memorias_conectadas);
	if (memoria_resultado != NULL) {
		return memoria_resultado;
	}
	// No hay ninguna memoria conectada, elegimos una al azar y la conectamos
	memoria_resultado = memoria_random(lista_memorias);
	if (memoria_resultado != NULL && conectar_memoria(memoria_resultado) < 0) {
		// No se pudo conectar la memoria elegida
		memoria_resultado = NULL;
	}
	return memoria_resultado;
}

int realizar_describe(struct global_describe_response *response) {
	struct describe_request request;
	int tamanio_buffer = get_max_msg_size();
	uint8_t buffer_local[tamanio_buffer];
	memoria_t *memoria;
	int error;
	if ((error = pthread_rwlock_rdlock(&memorias_rwlock)) != 0) {
		return -error;
	}
	if ((memoria = memoria_conectada_random(pool_memorias)) == NULL) {
		// Por el momento elegimos una memoria random para
		// realizar el describe
		pthread_rwlock_unlock(&memorias_rwlock);
		kernel_log_to_level(LOG_LEVEL_DEBUG, false,
				"Fallo al obtener metadata de tablas. Ninguna memoria conectada.");
		return -1;
	}
	if (pthread_mutex_lock(&memoria->mutex) != 0) {
		pthread_rwlock_unlock(&memorias_rwlock);
		return -1;
	}
	pthread_rwlock_unlock(&memorias_rwlock);
	if (init_describe_request(true, "", &request) < 0) {
		pthread_mutex_unlock(&memoria->mutex);
		kernel_log_to_level(LOG_LEVEL_TRACE, false,
				"Fallo al inicializar describe request");
		return -1;
	}
	if (enviar_y_recibir_respuesta(&request, memoria, buffer_local,
			tamanio_buffer) < 0) {
		pthread_mutex_unlock(&memoria->mutex);
		destroy(&request);
		return -1;
	}
	destroy(&request);
	pthread_mutex_unlock(&memoria->mutex);
	if(get_msg_id(buffer_local) == ERROR_MSG_ID) {
		mostrar_async(buffer_local);
		destroy(buffer_local);
		return -1;
	}
	memcpy(response, buffer_local, tamanio_buffer);
	return 0;
}

void registrar_metrica(memoria_t *memoria, void *mensaje, criterio_t criterio,
		uint32_t latency_usec) {
	tipo_operacion_t tipo_operacion;
	switch (get_msg_id(mensaje)) {
	case SELECT_REQUEST_ID:
		tipo_operacion = OP_SELECT;
		break;
	case INSERT_REQUEST_ID:
		tipo_operacion = OP_INSERT;
		break;
	default:
		return;
	}
	registrar_operacion(tipo_operacion, criterio, latency_usec,
			memoria->id_memoria);
}

int enviar_request(memoria_t *memoria, void *mensaje, void *respuesta,
		int tamanio_respuesta, criterio_t criterio) {
	if (tamanio_respuesta < get_max_msg_size()) {
		return -1;
	}
	if (pthread_mutex_lock(&memoria->mutex) != 0) {
		return -1;
	}
	clock_t inicio = clock();
	if (enviar_y_recibir_respuesta(mensaje, memoria, respuesta,
			tamanio_respuesta) < 0) {
		pthread_mutex_unlock(&memoria->mutex);
		return -1;
	}
	clock_t fin = clock();
	if (inicio != -1 && fin != -1) {
		uint32_t latency_usec = ((fin - inicio) * 1000000) / CLOCKS_PER_SEC;
		registrar_metrica(memoria, mensaje, criterio, latency_usec);
	}
	pthread_mutex_unlock(&memoria->mutex);
	return 0;
}

int enviar_request_a_sc(void *mensaje, void *respuesta, int tamanio_respuesta,
		int es_request_unitario) {
	if (list_size(criterio_sc) == 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, es_request_unitario,
				"Ninguna memoria asociada al criterio SC");
		return -1;
	}
	memoria_t *memoria = list_get(criterio_sc, 0);
	return enviar_request(memoria, mensaje, respuesta, tamanio_respuesta,
			CRITERIO_SC);
}

uint16_t hash_key(uint16_t key) {
	key += (key << 12);
	key ^= (key >> 6);
	key += (key << 4);
	key ^= (key >> 9);
	key += (key << 10);
	key ^= (key >> 7);
	key += (key << 13);
	key ^= (key >> 2);
	return key;
}

int enviar_request_a_shc(void *mensaje, uint16_t key, void *respuesta,
		int tamanio_respuesta, int es_request_unitario) {
	if (list_size(criterio_shc) == 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, es_request_unitario,
				"Ninguna memoria asociada al criterio SHC");
		return -1;
	}
	uint16_t hash = hash_key(key);
	memoria_t *memoria = list_get(criterio_shc, hash % list_size(criterio_shc));
	return enviar_request(memoria, mensaje, respuesta, tamanio_respuesta,
			CRITERIO_SHC);
}

int enviar_request_a_ec(void *mensaje, void *respuesta, int tamanio_respuesta,
		int es_request_unitario) {
	if (list_size(criterio_ec) == 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, es_request_unitario,
				"Ninguna memoria asociada al criterio EC");
		return -1;
	}
	memoria_t *memoria = memoria_random(criterio_ec);
	return enviar_request(memoria, mensaje, respuesta, tamanio_respuesta,
			CRITERIO_EC);
}

int obtener_criterio_y_key(void *mensaje, criterio_t *criterio, uint16_t *key,
		int es_request_unitario) {
	struct select_request *select_request;
	struct insert_request *insert_request;
	struct create_request *create_request;
	struct describe_request *describe_request;
	struct drop_request *drop_request;
	char *nombre_tabla = "";
	switch (get_msg_id(mensaje)) {
	case SELECT_REQUEST_ID:
		select_request = (struct select_request*) mensaje;
		nombre_tabla = select_request->tabla;
		*criterio = obtener_consistencia_tabla(select_request->tabla);
		*key = select_request->key;
		break;
	case INSERT_REQUEST_ID:
		insert_request = (struct insert_request*) mensaje;
		nombre_tabla = insert_request->tabla;
		*criterio = obtener_consistencia_tabla(insert_request->tabla);
		*key = insert_request->key;
		break;
	case CREATE_REQUEST_ID:
		create_request = (struct create_request*) mensaje;
		*criterio = create_request->consistencia;
		break;
	case DESCRIBE_REQUEST_ID:
		describe_request = (struct describe_request*) mensaje;
		nombre_tabla = describe_request->tabla;
		if (!describe_request->todas) {
			*criterio = obtener_consistencia_tabla(describe_request->tabla);
		} else {
			// Indicamos que no hay error
			*criterio = 0;
		}
		break;
	case DROP_REQUEST_ID:
		drop_request = (struct drop_request*) mensaje;
		nombre_tabla = drop_request->tabla;
		*criterio = obtener_consistencia_tabla(drop_request->tabla);
		break;
	default:
		return -1;
	}
	if (*criterio == -1) {
		kernel_log_to_level(LOG_LEVEL_ERROR, es_request_unitario,
				"Tabla desconocida: %s. Request fallido.", nombre_tabla);
		return TABLA_DESCONOCIDA;
	}
	return 0;
}

int enviar_request_a_memoria(void *mensaje, void *respuesta,
		int tamanio_respuesta, bool es_request_unitario) {
	int resultado = 0;
	criterio_t criterio;
	uint16_t key;

	if (obtener_criterio_y_key(mensaje, &criterio, &key, es_request_unitario)
			< 0) {
		return TABLA_DESCONOCIDA;
	}
	if (pthread_rwlock_rdlock(&memorias_rwlock) != 0) {
		return -1;
	}
	if (get_msg_id(mensaje) == JOURNAL_REQUEST_ID) {
		kernel_log_to_level(LOG_LEVEL_TRACE, false,
				"Realizando journal en todas las memorias");
		resultado = realizar_journal_a_lista(criterio_sc);
		resultado = realizar_journal_a_lista(criterio_shc) < 0 ? -1 : resultado;
		resultado = realizar_journal_a_lista(criterio_ec) < 0 ? -1 : resultado;
		pthread_rwlock_unlock(&memorias_rwlock);
		return resultado;
	}
	switch (criterio) {
	case CRITERIO_SC:
		resultado = enviar_request_a_sc(mensaje, respuesta, tamanio_respuesta,
				es_request_unitario);
		break;
	case CRITERIO_SHC:
		resultado = enviar_request_a_shc(mensaje, key, respuesta,
				tamanio_respuesta, es_request_unitario);
		break;
	case CRITERIO_EC:
		resultado = enviar_request_a_ec(mensaje, respuesta, tamanio_respuesta,
				es_request_unitario);
		break;
	default:
		resultado = -1;
	}
	pthread_rwlock_unlock(&memorias_rwlock);
	return resultado;
}
