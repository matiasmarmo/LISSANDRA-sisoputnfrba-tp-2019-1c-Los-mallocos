#include <stdio.h>
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

void kernel_log_to_level(t_log_level level, char *format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char *string = string_from_vformat(format, arguments);
	va_end(arguments);
	if (string == NULL) {
		return;
	}
	if (pthread_mutex_lock(&kernel_logger_mutex) != 0) {
		free(string);
		return;
	}
	switch (level) {
	case LOG_LEVEL_TRACE:
		log_trace(kernel_logger, string);
		break;
	case LOG_LEVEL_DEBUG:
		log_debug(kernel_logger, string);
		break;
	case LOG_LEVEL_INFO:
		log_info(kernel_logger, string);
		break;
	case LOG_LEVEL_WARNING:
		log_warning(kernel_logger, string);
		break;
	case LOG_LEVEL_ERROR:
		log_error(kernel_logger, string);
		break;
	}
	pthread_mutex_unlock(&kernel_logger_mutex);
	free(string);
}

void destruir_kernel_logger() {
	fflush(kernel_logger->file);
	log_destroy(kernel_logger);
}
