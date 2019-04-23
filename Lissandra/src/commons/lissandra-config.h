#ifndef LISSANDRA_CONFIG_H_
#define LISSANDRA_CONFIG_H_

#include <pthread.h>
#include <commons/config.h>

int inicializar_configuracion(t_config **configuracion, char *path, pthread_rwlock_t *lock);
int actualizar_configuracion(t_config **configuracion, char *path, pthread_rwlock_t *lock);
int destruir_configuracion(t_config *configuracion, pthread_rwlock_t *lock);

char* get_string_value(t_config *configuracion, char* key, pthread_rwlock_t *lock);
char** get_array_value(t_config *configuracion, char* key, pthread_rwlock_t *lock);
int get_int_value(t_config *configuracion, char* key, pthread_rwlock_t *lock);

#endif
