#include <string.h>
#include <pthread.h>
#include <commons/config.h>
#include <commons/string.h>

#include "lfs-config.h"
#include "../commons/lissandra-config.h"
#include "../commons/lissandra-inotify.h"

// Path al archivo de configuración
#define CONFIG_PATH "./lfs.conf"

// Claves del archivo de configuración
#define PUERTO_ESCUCHA "PUERTO_ESCUCHA"
#define PUNTO_MONTAJE "PUNTO_MONTAJE"
#define RETARDO "RETARDO"
#define TAMANIO_VALUE "TAMANIO_VALUE"
#define TIEMPO_DUMP "TIEMPO_DUMP"

t_config* configuracion_lfs;
pthread_rwlock_t lfs_config_lock = PTHREAD_RWLOCK_INITIALIZER;

char *get_punto_montaje();

void normalizar_punto_montaje() {
    
    // Si el punto de montaje no termina con '/', la agregamos

    char punto_montaje[256] = { 0 };
    strcpy(punto_montaje, get_punto_montaje());
    if(!string_ends_with(punto_montaje, "/")) {
        strcat(punto_montaje, "/");
    }
    pthread_rwlock_wrlock(&lfs_config_lock);
    config_set_value(configuracion_lfs, PUNTO_MONTAJE, punto_montaje);
    pthread_rwlock_unlock(&lfs_config_lock);
}

int inicializar_lfs_config() {
	int res = inicializar_configuracion(&configuracion_lfs, CONFIG_PATH, &lfs_config_lock);
    if(res == 0) {
        normalizar_punto_montaje();
    }
    return res;
}

int actualizar_lfs_config() {
	int res = actualizar_configuracion(&configuracion_lfs, CONFIG_PATH, &lfs_config_lock);
    if(res == 0) {
        normalizar_punto_montaje();
    }
    return res;
}

int inicializar_lfs_inotify(lissandra_thread_t* l_thread){
	return iniciar_inotify_watch(CONFIG_PATH, &actualizar_lfs_config, l_thread);
}

int destruir_lfs_config() {
	return destruir_configuracion(configuracion_lfs, &lfs_config_lock);
}

int get_puerto_escucha() {
	return get_int_value(configuracion_lfs, PUERTO_ESCUCHA, &lfs_config_lock);
}

char* get_punto_montaje() {
	return get_string_value(configuracion_lfs, PUNTO_MONTAJE, &lfs_config_lock);
}

int get_retardo() {
	return get_int_value(configuracion_lfs, RETARDO, &lfs_config_lock);
}

int get_tamanio_value() {
	return get_int_value(configuracion_lfs, TAMANIO_VALUE, &lfs_config_lock);
}

int get_tiempo_dump() {
	return get_int_value(configuracion_lfs, TIEMPO_DUMP, &lfs_config_lock);
}
