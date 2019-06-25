#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "../commons/comunicacion/sockets.h"
#include "../commons/comunicacion/protocol.h"
#include "memoria-config.h"
#include "memoria-main.h"

pthread_mutex_t mutex_conexion_lfs = PTHREAD_MUTEX_INITIALIZER;
int socket_lfs = -1;

int conectar_lfs() {
    char *ip_lfs = get_ip_file_system();
    char puerto_lfs[6] = { 0 };
    int puerto_lfs_int = get_puerto_file_system();
    sprintf(puerto_lfs, "%d", puerto_lfs_int);
    uint8_t buffer[get_max_msg_size()];
    memset(buffer, 0, get_max_msg_size());
    if(pthread_mutex_lock(&mutex_conexion_lfs) != 0) {
        return -1;
    }
    socket_lfs = create_socket_client(ip_lfs, puerto_lfs, FLAG_NONE);
    if(socket_lfs < 0) {
        pthread_mutex_unlock(&mutex_conexion_lfs);
        return -1;
    }
    if(recv_msg(socket_lfs, buffer, get_max_msg_size()) < 0) {
        close(socket_lfs);
        pthread_mutex_unlock(&mutex_conexion_lfs);
        return -1;
    }
    if(get_msg_id(buffer) == ERROR_MSG_ID) {
        close(socket_lfs);
        pthread_mutex_unlock(&mutex_conexion_lfs);
        return -1;   
    }
    struct lfs_handshake *mensaje = (struct lfs_handshake*) buffer;
    set_tamanio_value(mensaje->tamanio_max_value);
    pthread_mutex_unlock(&mutex_conexion_lfs);
    return 0;
}

int desconectar_lfs() {
    if(pthread_mutex_lock(&mutex_conexion_lfs) != 0) {
        return -1;
    }
    close(socket_lfs);
    pthread_mutex_unlock(&mutex_conexion_lfs);
    return 0;
}

int enviar_mensaje_lfs(void *request, void *respuesta) {
    if(pthread_mutex_lock(&mutex_conexion_lfs) != 0) {
        return -1;
    }
    if(send_msg(socket_lfs, request) < 0) {
    	memoria_log_to_level(LOG_LEVEL_ERROR, false,
    		"Se perdio la conexion");
        pthread_mutex_unlock(&mutex_conexion_lfs);
        return -1;
    }
    if(recv_msg(socket_lfs, respuesta, get_max_msg_size()) < 0) {
    	memoria_log_to_level(LOG_LEVEL_ERROR, false,
    		"Se perdio la conexion");
        pthread_mutex_unlock(&mutex_conexion_lfs);
        return -1;
    }
    pthread_mutex_unlock(&mutex_conexion_lfs);
    return 0;
}
