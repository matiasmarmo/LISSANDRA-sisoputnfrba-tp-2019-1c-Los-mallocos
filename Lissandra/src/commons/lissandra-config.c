#include <pthread.h>
#include <commons/config.h>

#include "lissandra-config.h"

int bloquear_mutex(pthread_mutex_t *mutex) {
	return pthread_mutex_lock(mutex);
}

int desbloquear_mutex(pthread_mutex_t *mutex) {
	return pthread_mutex_unlock(mutex);
}

int cargar_configuracion(t_config **configuracion, char *path) {
	*configuracion = config_create(path);
	return *configuracion == NULL ? -1 : 0;
}

int inicializar_configuracion(t_config **configuracion, char *path, pthread_mutex_t *mutex) {
	int error, resultado;
	if((error = bloquear_mutex(mutex)) != 0) {
		return -error;
	}
	resultado = cargar_configuracion(configuracion, path);
	desbloquear_mutex(mutex);
	return resultado;
}

int actualizar_configuracion(t_config **configuracion, char *path, pthread_mutex_t *mutex) {
	int error, resultado;
	if((error = bloquear_mutex(mutex)) != 0) {
		return -error;
	}
	if(*configuracion != NULL) {
		config_destroy(*configuracion);
	}
	resultado = cargar_configuracion(configuracion, path);
	desbloquear_mutex(mutex);
	return resultado;
}

int destruir_configuracion(t_config *configuracion, pthread_mutex_t *mutex) {
	int error;
	if((error = bloquear_mutex(mutex)) != 0) {
		return -error;
	}
	if(configuracion != NULL) {
		config_destroy(configuracion);
	}
	desbloquear_mutex(mutex);
	return 0;
}

char* get_string_value(t_config *configuracion, char* key, pthread_mutex_t *mutex) {
	int error;
	if(configuracion == NULL) {
		return NULL;
	}
	if((error = bloquear_mutex(mutex)) != 0) {
		return NULL;
	}
	char* resultado = config_get_string_value(configuracion, key);
	desbloquear_mutex(mutex);
	return resultado;
}

char** get_array_value(t_config *configuracion, char* key, pthread_mutex_t *mutex) {
	int error;
	if(configuracion == NULL) {
		return NULL;
	}
	if((error = bloquear_mutex(mutex)) != 0) {
		return NULL;
	}
	char** resultado = config_get_array_value(configuracion, key);
	desbloquear_mutex(mutex);
	return resultado;
}

int get_int_value(t_config *configuracion, char* key, pthread_mutex_t *mutex) {
	int error;
	if(configuracion == NULL) {
		return -1;
	}
	if((error = bloquear_mutex(mutex)) != 0) {
		return -error;
	}
	int resultado = config_get_int_value(configuracion, key);
	desbloquear_mutex(mutex);
	return resultado;
}
