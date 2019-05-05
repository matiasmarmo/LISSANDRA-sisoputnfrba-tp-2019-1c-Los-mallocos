#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>
#include <commons/log.h>
#include <commons/string.h>

#include "../commons/lissandra-logger.h"
#include "../commons/consola/consola.h"
#include "memoria-logger.h"

#define MEMORIA_PROGRAM "memorial"
#define MEMORIA_LOG_PATH "memoria.log"

pthread_mutex_t memoria_logger_mutex = PTHREAD_MUTEX_INITIALIZER;
t_log *memoria_logger;

int inicializar_memoria_logger() {
	memoria_logger = lissandra_log_create(MEMORIA_LOG_PATH, MEMORIA_PROGRAM);
	return memoria_logger == NULL ? -1 : 0;
}

void memoria_log_to_level(t_log_level level, bool es_consola, char *format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char *string = string_from_vformat(format, arguments);
	va_end(arguments);
	if (string == NULL) {
		return;
	}
	if (pthread_mutex_lock(&memoria_logger_mutex) != 0) {
		free(string);
		return;
	}
	lissandra_log_to_level(memoria_logger, level, string);
	if(es_consola) {
		imprimir_async(string);
	}
	pthread_mutex_unlock(&memoria_logger_mutex);
	free(string);
}

void destruir_memoria_logger() {
	fflush(memoria_logger->file);
	log_destroy(memoria_logger);
}

