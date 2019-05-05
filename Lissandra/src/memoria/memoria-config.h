#ifndef LISSANDRA_SRC_MEMORIA_MEMORIA_CONFIG_H_
#define LISSANDRA_SRC_MEMORIA_MEMORIA_CONFIG_H_

#include "../commons/lissandra-threads.h"

int inicializar_memoria_config();
int inicializar_memoria_inotify(lissandra_thread_t*);
int actualizar_memoria_config();
int destruir_memoria_config();

int get_puerto_escucha_mem();
char* get_ip_file_system();
int get_puerto_file_system();
char** get_ip_seeds();
char** get_puertos_seed(); // luego cuando necesitemos usar dicho valor haremos la conversion a un array numérico
int get_retardo_memoria_principal();
int get_retardo_acceso_file_system();
int get_tamanio_memoria();
int get_tiempo_journal();
int get_tiempo_gossiping();
int get_numero_memoria();

#endif
