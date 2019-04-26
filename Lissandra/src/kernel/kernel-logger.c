#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <commons/log.h>
#include <commons/string.h>

#include "../commons/lissandra-logger.h"
#include "kernel-logger.h"

#define KERNEL_PROGRAM "kernel"
#define KERNEL_LOG_PATH "kernel.log"

pthread_mutex_t kernel_logger_mutex = PTHREAD_MUTEX_INITIALIZER;
t_log *kernel_logger;

int inicializar_kernel_logger() {
	kernel_logger = lissandra_log_create(KERNEL_LOG_PATH, KERNEL_PROGRAM);
	return kernel_logger == NULL ? -1 : 0;
}

t_log *get_kernel_logger() {
	return kernel_logger;
}

pthread_mutex_t *get_kernel_logger_mutex() {
	return &kernel_logger_mutex;
}

#define kernel_log(kernel_log_func, log_func) \
	void kernel_log_func(char *string) { \
		if(kernel_logger != NULL && pthread_mutex_lock(&kernel_logger_mutex) == 0){ \
			log_func(kernel_logger, string); \
			pthread_mutex_unlock(&kernel_logger_mutex); \
		} \
	} \

kernel_log(kernel_log_trace, log_trace);
kernel_log(kernel_log_debug, log_debug);
kernel_log(kernel_log_info, log_info);
kernel_log(kernel_log_warning, log_warning);
kernel_log(kernel_log_error, log_error);

#undef kernel_log

void destruir_kernel_logger() {
	log_destroy(kernel_logger);
}
