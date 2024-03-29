#include <pthread.h>
#include <commons/config.h>

#include "lissandra-config.h"

int bloquear_lock_lectura(pthread_rwlock_t *lock) {
	return pthread_rwlock_rdlock(lock);
}

int bloquear_lock_escritura(pthread_rwlock_t *lock) {
	return pthread_rwlock_wrlock(lock);
}

int desbloquear_lock(pthread_rwlock_t *lock) {
	return pthread_rwlock_unlock(lock);
}

int cargar_configuracion(t_config **configuracion, char *path) {
	*configuracion = config_create(path);
	return *configuracion == NULL ? -1 : 0;
}

int inicializar_configuracion(t_config **configuracion, char *path, pthread_rwlock_t *lock) {
	int error, resultado;
	if((error = bloquear_lock_escritura(lock)) != 0) {
		return -error;
	}
	resultado = cargar_configuracion(configuracion, path);
	desbloquear_lock(lock);
	return resultado;
}

int actualizar_configuracion(t_config **configuracion, char *path, pthread_rwlock_t *lock) {
	int error, resultado;
	if((error = bloquear_lock_escritura(lock)) != 0) {
		return -error;
	}
	if(*configuracion != NULL) {
		config_destroy(*configuracion);
	}
	resultado = cargar_configuracion(configuracion, path);
	desbloquear_lock(lock);
	return resultado;
}

int destruir_configuracion(t_config *configuracion, pthread_rwlock_t *lock) {
	int error;
	if((error = bloquear_lock_escritura(lock)) != 0) {
		return -error;
	}
	if(configuracion != NULL) {
		config_destroy(configuracion);
	}
	desbloquear_lock(lock);
	return 0;
}

char* get_string_value(t_config *configuracion, char* key, pthread_rwlock_t *lock) {
	int error;
	if(configuracion == NULL) {
		return NULL;
	}
	if((error = bloquear_lock_lectura(lock)) != 0) {
		return NULL;
	}
	char* resultado = config_get_string_value(configuracion, key);
	desbloquear_lock(lock);
	return resultado;
}

char** get_array_value(t_config *configuracion, char* key, pthread_rwlock_t *lock) {
	int error;
	if(configuracion == NULL) {
		return NULL;
	}
	if((error = bloquear_lock_lectura(lock)) != 0) {
		return NULL;
	}
	char** resultado = config_get_array_value(configuracion, key);
	desbloquear_lock(lock);
	return resultado;
}

int get_int_value(t_config *configuracion, char* key, pthread_rwlock_t *lock) {
	int error;
	if(configuracion == NULL) {
		return -1;
	}
	if((error = bloquear_lock_lectura(lock)) != 0) {
		return -error;
	}
	int resultado = config_get_int_value(configuracion, key);
	desbloquear_lock(lock);
	return resultado;
}
