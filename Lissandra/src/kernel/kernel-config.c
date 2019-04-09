#include <pthread.h>
#include <commons/config.h>

#include "kernel-config.h"
#include "../commons/lissandra-config.h"

// Path al archivo de configuración
#define CONFIG_PATH "./kernel.conf"

// Claves del archivo de configuración
#define IP_MEMORIA "IP_MEMORIA"
#define PUERTO_MEMORIA "PUERTO_MEMORIA"
#define QUANTUM "QUANTUM"
#define MULTIPROCESAMIENTO "MULTIPROCESAMIENTO"
#define METADATA_REFRESH "METADATA_REFRESH"
#define SLEEP_EJECUCION "SLEEP_EJECUCION"

t_config* configuracion;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int inicializar_kernel_config() {
	return inicializar_configuracion(&configuracion, CONFIG_PATH, &lock);
}

int actualizar_kernel_config() {
	return actualizar_configuracion(&configuracion, CONFIG_PATH, &lock);
}

int destruir_kernel_config() {
	return destruir_configuracion(configuracion, &lock);
}

char* get_ip_memoria() {
	return get_string_value(configuracion, IP_MEMORIA, &lock);
}

int get_puerto_memoria() {
	return get_int_value(configuracion, PUERTO_MEMORIA, &lock);
}

int get_quantum() {
	return get_int_value(configuracion, QUANTUM, &lock);
}

int get_multiprocesamiento() {
	return get_int_value(configuracion, MULTIPROCESAMIENTO, &lock);
}

int get_refresh_metadata() {
	return get_int_value(configuracion, METADATA_REFRESH, &lock);
}

int get_retardo_ejecucion() {
	return get_int_value(configuracion, SLEEP_EJECUCION, &lock);
}
