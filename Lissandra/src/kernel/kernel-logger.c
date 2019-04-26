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

void kernel_log_to_level(t_log_level level, char *format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char *string = string_from_vformat(format, arguments);
	va_end(arguments);
	if(string == NULL) {
		return;
	}
	switch (level) {
	case LOG_LEVEL_TRACE:
		kernel_log_trace(string);
		break;
	case LOG_LEVEL_DEBUG:
		kernel_log_debug(string);
		break;
	case LOG_LEVEL_INFO:
		kernel_log_info(string);
		break;
	case LOG_LEVEL_WARNING:
		kernel_log_warning(string);
		break;
	case LOG_LEVEL_ERROR:
		kernel_log_error(string);
		break;
	}
	free(string);
}

void destruir_kernel_logger() {
	log_destroy(kernel_logger);
}
