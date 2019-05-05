#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

#include "../commons/lissandra-threads.h"

int inicializar_lfs_config();
int actualizar_lfs_config();
int inicializar_lfs_inotify(lissandra_thread_t*);
int destruir_lfs_config();

int get_puerto_escucha();
char* get_punto_montaje();
int get_retardo();
int get_tamanio_value();
int get_tiempo_dump();

#endif
