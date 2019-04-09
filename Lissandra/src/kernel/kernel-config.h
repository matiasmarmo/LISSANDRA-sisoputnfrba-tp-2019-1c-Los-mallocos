#ifndef KERNEL_CONFIG_H_
#define KERNEL_CONFIG_H_

int inicializar_kernel_config();
int actualizar_kernel_config();
int destruir_kernel_config();

char* get_ip_memoria();
int get_puerto_memoria();
int get_quantum();
int get_multiprocesamiento();
int get_refresh_metadata();
int get_retardo_ejecucion();

#endif
