#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <commons/log.h>
#include "../commons/comunicacion/sockets.h"
#include "../commons/comunicacion/protocol.h"
#include "memoria-config.h"
#include "memoria-logger.h"
#include "conexion-lfs.h"
#include "memoria-main.h"

pthread_mutex_t mutex_conexion_lfs = PTHREAD_MUTEX_INITIALIZER;
int socket_lfs = -1;

int _conectar_lfs() {
    char *ip_lfs = get_ip_file_system();
    char puerto_lfs[6] = { 0 };
    int puerto_lfs_int = get_puerto_file_system();
    sprintf(puerto_lfs, "%d", puerto_lfs_int);
    uint8_t buffer[get_max_msg_size()];
    memset(buffer, 0, get_max_msg_size());
    socket_lfs = create_socket_client(ip_lfs, puerto_lfs, FLAG_NONE);
    if(socket_lfs < 0) {
        return -1;
    }
    if(recv_msg(socket_lfs, buffer, get_max_msg_size()) < 0) {
        close(socket_lfs);
        return -1;
    }
    if(get_msg_id(buffer) == ERROR_MSG_ID) {
        close(socket_lfs);
        return -1;   
    }
    struct lfs_handshake *mensaje = (struct lfs_handshake*) buffer;
    set_tamanio_value(mensaje->tamanio_max_value);
    return 0;
}

int conectar_lfs() {
    int resultado;
    if(pthread_mutex_lock(&mutex_conexion_lfs) != 0) {
        return -1;
    }
    resultado = _conectar_lfs();
    pthread_mutex_unlock(&mutex_conexion_lfs);
    return resultado;
}

int desconectar_lfs() {
    if(pthread_mutex_lock(&mutex_conexion_lfs) != 0) {
        return -1;
    }
    close(socket_lfs);
    pthread_mutex_unlock(&mutex_conexion_lfs);
    return 0;
}

int reconectar_lfs() {
    if(_conectar_lfs() >= 0) {
        memoria_log_to_level(LOG_LEVEL_INFO, true, "Reconectado con el file system");
        return 0;
    }
    memoria_log_to_level(LOG_LEVEL_ERROR, true, "No se pudo reconectar con el file system, finalizando memoria");
    return -1;
}

int enviar_mensaje_lfs(void *request, void *respuesta, bool should_sleep) {
    if(pthread_mutex_lock(&mutex_conexion_lfs) != 0) {
        return -1;
    }
    if(socket_lfs == -1 && _conectar_lfs() < 0) {
        pthread_mutex_unlock(&mutex_conexion_lfs);
        init_error_msg(0, "Error de conexion entre la memoria y el lfs", respuesta);
        return 0;
    }
    if(send_msg(socket_lfs, request) < 0) {
    	memoria_log_to_level(LOG_LEVEL_ERROR, true,
    		"Se perdio la conexion con el file system, intentando reconectar");
        socket_lfs = -1;
        if(reconectar_lfs() < 0) {
            pthread_mutex_unlock(&mutex_conexion_lfs);
            init_error_msg(0, "Error de conexion entre la memoria y el lfs", respuesta);
            return 0;
        }
    }
    if(recv_msg(socket_lfs, respuesta, get_max_msg_size()) < 0) {
    	memoria_log_to_level(LOG_LEVEL_ERROR, true,
    		"Se perdio la conexion con el file system, intentando reconectar");
        socket_lfs = -1;
        if(reconectar_lfs() < 0) {
            pthread_mutex_unlock(&mutex_conexion_lfs);
            init_error_msg(0, "Error de conexion entre la memoria y el lfs", respuesta);
            return 0;
        }
    }
    pthread_mutex_unlock(&mutex_conexion_lfs);
    if(should_sleep) {
        usleep(get_retardo_acceso_file_system() * 1000);
    }
    return 0;
}
