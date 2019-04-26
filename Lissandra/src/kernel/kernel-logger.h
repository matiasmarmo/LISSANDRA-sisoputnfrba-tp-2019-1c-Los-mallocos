#ifndef KERNEL_LOGGER_H_
#define KERNEL_LOGGER_H_

#include <commons/log.h>

int inicializar_kernel_logger();
t_log *get_kernel_logger();
pthread_mutex_t *get_kernel_logger_mutex();
void destruir_kernel_logger();

void kernel_log_trace(char*);
void kernel_log_debug(char*);
void kernel_log_info(char*);
void kernel_log_warning(char*);
void kernel_log_error(char*);

#endif
