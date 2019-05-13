#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "../commons/lissandra-threads.h"
#include "lfs-logger.h"
#include "lfs-config.h"
#include "lfs-servidor.h"
#include "lfs-main.h"

pthread_mutex_t lfs_main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t lfs_main_cond = PTHREAD_COND_INITIALIZER;

void finalizar_lfs() {
	pthread_mutex_lock(&lfs_main_mutex);
	pthread_cond_signal(&lfs_main_cond);
	pthread_mutex_unlock(&lfs_main_mutex);
}

void inicializar_lfs() {
	if (inicializar_lfs_logger() < 0) {
		exit(EXIT_FAILURE);
	}
	if (inicializar_lfs_config() < 0) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false,
				"Error al inicializar archivo de configuración. Abortando.");
		destruir_lfs_logger();
		exit(EXIT_FAILURE);
	}
}

void liberar_recursos_lfs() {
	destruir_lfs_config();
	destruir_lfs_logger();
}

void finalizar_thread(lissandra_thread_t *l_thread) {
	l_thread_solicitar_finalizacion(l_thread);
	l_thread_join(l_thread, NULL);
}

void finalizar_thread_si_se_creo(lissandra_thread_t *l_thread, int create_ret) {
	// Si el create al thread retornó 0 (fue exitoso), la finalizamos.
	if (create_ret == 0) {
		finalizar_thread(l_thread);
	}
}

int main() {
	// inicio lissandra
	int rets[CANTIDAD_PROCESOS];
	inicializar_lfs();
	lissandra_thread_t servidor, inotify;

	rets[0] = l_thread_create(&servidor, &correr_servidor, NULL);
	rets[1] = inicializar_lfs_inotify(&inotify);

	for (int i = 0; i < CANTIDAD_PROCESOS; i++) {
		if (rets[i] != 0) {
			finalizar_thread_si_se_creo(&servidor, rets[0]);
			finalizar_thread_si_se_creo(&inotify, rets[1]);
			liberar_recursos_lfs();
			exit(EXIT_FAILURE);
		}
	}

	lfs_log_to_level(LOG_LEVEL_INFO, false, "Todos los módulos inicializados.");

	pthread_mutex_lock(&lfs_main_mutex);
	pthread_cond_wait(&lfs_main_cond, &lfs_main_mutex);
	pthread_mutex_unlock(&lfs_main_mutex);

	finalizar_thread(&servidor);
	finalizar_thread(&inotify);
	lfs_log_to_level(LOG_LEVEL_INFO, false, "Finalizando.");
	liberar_recursos_lfs();
	return 0;

}
