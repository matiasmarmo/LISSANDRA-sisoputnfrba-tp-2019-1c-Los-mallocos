#ifndef LISSANDRA_CONFIG_H_
#define LISSANDRA_CONFIG_H_

#include <pthread.h>
#include <commons/config.h>

int inicializar_configuracion(t_config **configuracion, char *path, pthread_mutex_t *mutex);
int actualizar_configuracion(t_config **configuracion, char *path, pthread_mutex_t *mutex);
int destruir_configuracion(t_config *configuracion, pthread_mutex_t *mutex);

char* get_string_value(t_config *configuracion, char* key, pthread_mutex_t *mutex);
char** get_array_value(t_config *configuracion, char* key, pthread_mutex_t *mutex);
int get_int_value(t_config *configuracion, char* key, pthread_mutex_t *mutex);

#endif
