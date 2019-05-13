#include <pthread.h>
#include <commons/config.h>

#include "kernel-config.h"
#include "../commons/lissandra-config.h"
#include "../commons/lissandra-inotify.h"

// Path al archivo de configuración
#define CONFIG_PATH "./kernel.conf"

// Claves del archivo de configuración
#define IP_MEMORIA "IP_MEMORIA"
#define PUERTO_MEMORIA "PUERTO_MEMORIA"
#define NUMERO_MEMORIA "NUMERO_MEMORIA"
#define QUANTUM "QUANTUM"
#define MULTIPROCESAMIENTO "MULTIPROCESAMIENTO"
#define METADATA_REFRESH "METADATA_REFRESH"
#define MEMORIAS_REFRESH "MEMORIAS_REFRESH"
#define SLEEP_EJECUCION "SLEEP_EJECUCION"

t_config* kernel_configuracion;
pthread_rwlock_t kernel_config_lock = PTHREAD_RWLOCK_INITIALIZER;

int inicializar_kernel_config() {
	return inicializar_configuracion(&kernel_configuracion, CONFIG_PATH, &kernel_config_lock);
}

int actualizar_kernel_config() {
	return actualizar_configuracion(&kernel_configuracion, CONFIG_PATH, &kernel_config_lock);
}

int inicializar_kernel_inotify(lissandra_thread_t* l_thread) {
	return iniciar_inotify_watch(CONFIG_PATH, &actualizar_kernel_config, l_thread);
}

int destruir_kernel_config() {
	return destruir_configuracion(kernel_configuracion, &kernel_config_lock);
}

char* get_ip_memoria() {
	return get_string_value(kernel_configuracion, IP_MEMORIA, &kernel_config_lock);
}

int get_puerto_memoria() {
	return get_int_value(kernel_configuracion, PUERTO_MEMORIA, &kernel_config_lock);
}

int get_numero_memoria() {
	return get_int_value(kernel_configuracion, NUMERO_MEMORIA, &kernel_config_lock);
}

int get_quantum() {
	return get_int_value(kernel_configuracion, QUANTUM, &kernel_config_lock);
}

int get_multiprocesamiento() {
	return get_int_value(kernel_configuracion, MULTIPROCESAMIENTO, &kernel_config_lock);
}

int get_refresh_metadata() {
	return get_int_value(kernel_configuracion, METADATA_REFRESH, &kernel_config_lock);
}

int get_refresh_memorias() {
	return get_int_value(kernel_configuracion, MEMORIAS_REFRESH, &kernel_config_lock);
}

int get_retardo_ejecucion() {
	return get_int_value(kernel_configuracion, SLEEP_EJECUCION, &kernel_config_lock);
}
