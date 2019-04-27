#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>
#include <commons/log.h>
#include <commons/string.h>

#include "../commons/lissandra-logger.h"
#include "../commons/consola/consola.h"
#include "kernel-logger.h"

#define KERNEL_PROGRAM "kernel"
#define KERNEL_LOG_PATH "kernel.log"

pthread_mutex_t kernel_logger_mutex = PTHREAD_MUTEX_INITIALIZER;
t_log *kernel_logger;

int inicializar_kernel_logger() {
	kernel_logger = lissandra_log_create(KERNEL_LOG_PATH, KERNEL_PROGRAM);
	return kernel_logger == NULL ? -1 : 0;
}

void kernel_log_to_level(t_log_level level, bool es_consola, char *format, ...) {
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
	lissandra_log_to_level(kernel_logger, level, string);
	if(es_consola) {
		imprimir_async(string);
	}
	pthread_mutex_unlock(&kernel_logger_mutex);
	free(string);
}

void destruir_kernel_logger() {
	fflush(kernel_logger->file);
	log_destroy(kernel_logger);
}
