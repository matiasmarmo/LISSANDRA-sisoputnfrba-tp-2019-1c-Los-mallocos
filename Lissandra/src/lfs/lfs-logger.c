#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdarg.h>
#include <commons/log.h>
#include <commons/string.h>

#include "../commons/lissandra-logger.h"
#include "../commons/consola/consola.h"
#include "lfs-logger.h"

#define LFS_PROGRAM "lfs"
#define LFS_LOG_PATH "lfs.log"

pthread_mutex_t lfs_logger_mutex = PTHREAD_MUTEX_INITIALIZER;
t_log *lfs_logger;

int inicializar_lfs_logger() {
	lfs_logger = lissandra_log_create(LFS_LOG_PATH, LFS_PROGRAM);
	return lfs_logger == NULL ? -1 : 0;
}

void lfs_log_to_level(t_log_level level, bool es_consola, char *format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char *string = string_from_vformat(format, arguments);
	va_end(arguments);
	if (string == NULL) {
		return;
	}
	if (pthread_mutex_lock(&lfs_logger_mutex) != 0) {
		free(string);
		return;
	}
	lissandra_log_to_level(lfs_logger, level, string);
	if(es_consola) {
		imprimir_async(string);
	}
	pthread_mutex_unlock(&lfs_logger_mutex);
	free(string);
}

void destruir_lfs_logger() {
	fflush(lfs_logger->file);
	log_destroy(lfs_logger);
}
