#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

#include "../commons/lissandra-threads.h"

int inicializar_kernel_config();
int inicializar_kernel_inotify(lissandra_thread_t*);
int actualizar_kernel_config();
int destruir_kernel_config();

char* get_ip_memoria();
int get_puerto_memoria();
int get_numero_memoria();
int get_quantum();
int get_multiprocesamiento();
int get_refresh_metadata();
int get_refresh_memorias();
int get_retardo_ejecucion();

#endif
