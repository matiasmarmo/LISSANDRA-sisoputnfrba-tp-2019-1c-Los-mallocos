#include <pthread.h>
#include <commons/config.h>

#include "memoria-config.h"
#include "../commons/lissandra-config.h"
#include "../commons/lissandra-inotify.h"

// Path al archivo de configuración
#define CONFIG_PATH "./memoria.conf"

// Claves del archivo de configuración
#define IP_FS "IP_FS"
#define PUERTO "PUERTO_ESCUCHA"
#define PUERTO_FS "PUERTO_FS"
#define IP_SEEDS "IP_SEEDS"
#define PUERTO_SEEDS "PUERTO_SEEDS"
#define RETARDO_MEM "RETARDO_MEM"
#define RETARDO_FS "RETARDO_FS"
#define TAM_MEM "TAM_MEM" //
#define RETARDO_JOURNAL "RETARDO_JOURNAL"
#define RETARDO_GOSSIPING "RETARDO_GOSSIPING"
#define MEMORY_NUMBER "MEMORY_NUMBER"


t_config* configuracion_memoria;
pthread_rwlock_t memoria_config_lock = PTHREAD_RWLOCK_INITIALIZER;

int inicializar_memoria_config() {
	return inicializar_configuracion(&configuracion_memoria, CONFIG_PATH, &memoria_config_lock);
}

int actualizar_memoria_config() {
	return actualizar_configuracion(&configuracion_memoria, CONFIG_PATH, &memoria_config_lock);
}

int inicializar_memoria_inotify(lissandra_thread_t* l_thread) {
	return iniciar_inotify_watch(CONFIG_PATH, &actualizar_memoria_config, l_thread);
}

int destruir_memoria_config() {
	return destruir_configuracion(configuracion_memoria, &memoria_config_lock);
}

int get_puerto_escucha_mem() {
	return get_int_value(configuracion_memoria, PUERTO, &memoria_config_lock);
}

char* get_ip_file_system() {
	return get_string_value(configuracion_memoria, IP_FS, &memoria_config_lock);
}

int get_puerto_file_system() {
	return get_int_value(configuracion_memoria, PUERTO_FS, &memoria_config_lock);
}

char** get_ip_seeds() {
	return get_array_value(configuracion_memoria, IP_SEEDS, &memoria_config_lock);
}

char** get_puertos_seed() {
	return get_array_value(configuracion_memoria, PUERTO_SEEDS, &memoria_config_lock);
}

int get_retardo_memoria_principal() {
	return get_int_value(configuracion_memoria, RETARDO_MEM, &memoria_config_lock);
}

int get_retardo_acceso_file_system() {
	return get_int_value(configuracion_memoria, RETARDO_FS, &memoria_config_lock);
}

int get_tamanio_memoria() {
	return get_int_value(configuracion_memoria, TAM_MEM, &memoria_config_lock);
}

int get_tiempo_journal() {
	return get_int_value(configuracion_memoria, RETARDO_JOURNAL, &memoria_config_lock);
}

int get_tiempo_gossiping() {
	return get_int_value(configuracion_memoria, RETARDO_GOSSIPING, &memoria_config_lock);
}

int get_numero_memoria() {
	return get_int_value(configuracion_memoria, MEMORY_NUMBER, &memoria_config_lock);
}
