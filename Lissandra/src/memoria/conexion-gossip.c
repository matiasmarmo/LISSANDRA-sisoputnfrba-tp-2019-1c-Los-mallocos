#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "../commons/comunicacion/sockets.h"
#include "../commons/comunicacion/protocol.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-main.h"
#include "memoria-gossip.h"

pthread_mutex_t mutex_conexion_gossip = PTHREAD_MUTEX_INITIALIZER;
int socket_gossip = -1;

int conectar_gossip() {
    char *ip_gossip = get_ip_file_system(); // DEBO OBTENER IP MEMORIA A CONECTARME
    char puerto_gossip[6] = { 0 };
    int puerto_gossip_int = get_puerto_escucha_mem(); // DEBO OBTENER IP PROPIA!!!! -- seeds _

    sprintf(puerto_gossip, "%d", puerto_gossip_int);
    uint8_t buffer[get_max_msg_size()];
    memset(buffer, 0, get_max_msg_size());
    if(pthread_mutex_lock(&mutex_conexion_gossip) != 0) {
        return -1;
    }
    socket_gossip = create_socket_client(ip_gossip, puerto_gossip, FLAG_NONE);
    if(socket_gossip < 0) {
        pthread_mutex_unlock(&mutex_conexion_gossip);
        return -1;
    }
    if(recv_msg(socket_gossip, buffer, get_max_msg_size()) < 0) {
        close(socket_gossip);
        pthread_mutex_unlock(&mutex_conexion_gossip);
        return -1;
    }
    if(get_msg_id(buffer) == ERROR_MSG_ID) {
        close(socket_gossip);
        pthread_mutex_unlock(&mutex_conexion_gossip);
        return -1;
    }
    struct gossip *gossip_mensaje = (struct gossip*) buffer;
    if(realizar_gossip(gossip_mensaje->id) < 0){
    	memoria_log_to_level(LOG_LEVEL_TRACE, false,
    		"Error al realizar el gossip");
    	return NULL;
    }

    pthread_mutex_unlock(&mutex_conexion_gossip);
    return 0;
}

int desconectar_gossip() {
    if(pthread_mutex_lock(&mutex_conexion_gossip) != 0) {
        return -1;
    }
    close(socket_gossip);
    pthread_mutex_unlock(&mutex_conexion_gossip);
    return 0;
}

int enviar_respuesta_gossip(void *respuesta) {
    if(pthread_mutex_lock(&mutex_conexion_gossip) != 0) {
        return -1;
    }
    if(recv_msg(socket_gossip, respuesta, get_max_msg_size()) < 0) {
        pthread_mutex_unlock(&mutex_conexion_gossip);
        return -1;
    }
    pthread_mutex_unlock(&mutex_conexion_gossip);
    return 0;
}
