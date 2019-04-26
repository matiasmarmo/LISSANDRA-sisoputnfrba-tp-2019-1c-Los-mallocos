#ifndef KERNEL_LOGGER_H_
#define KERNEL_LOGGER_H_

#include <commons/log.h>

int inicializar_kernel_logger();
void destruir_kernel_logger();

void kernel_log_to_level(t_log_level, bool, char*, ...);

#endif
