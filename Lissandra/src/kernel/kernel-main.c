#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "../commons/lissandra-threads.h"
#include "pool-memorias/manager-memorias.h"
#include "pool-memorias/metadata-tablas.h"
#include "ejecucion/planificador.h"
#include "kernel-logger.h"
#include "kernel-config.h"
#include "kernel-consola.h"
#include "metricas.h"
#include "kernel-main.h"

pthread_mutex_t kernel_main_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t kernel_main_cond = PTHREAD_COND_INITIALIZER;

void finalizar_kernel() {
	pthread_cond_signal(&kernel_main_cond);
}

void inicializar_kernel() {
	inicializar_metricas();
	if (inicializar_kernel_logger() < 0) {
		destruir_metricas();
		exit(EXIT_FAILURE);
	}
	if (inicializar_kernel_config() < 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, "Error al inicializar archivo de configuración. Abortando.");
		destruir_metricas();
		destruir_kernel_logger();
		exit(EXIT_FAILURE);
	}
	if (inicializar_memorias() < 0) {
		kernel_log_to_level(LOG_LEVEL_ERROR, "Error al inicializar el pool de memorias. Abortando.");
		destruir_metricas();
		destruir_kernel_config();
		destruir_kernel_logger();
		exit(EXIT_FAILURE);
	}
}

void liberar_recursos_kernel() {
	destruir_metricas();
	destruir_kernel_config();
	destruir_memorias();
	destruir_kernel_logger();
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
	inicializar_kernel();
	int rets[5];
	lissandra_thread_t planificador, consola, inotify;
	lissandra_thread_periodic_t metadata_updater, memorias_updater;
	rets[0] = l_thread_create(&planificador, &correr_planificador, NULL);
	rets[1] = l_thread_create(&consola, &correr_consola, NULL);
	rets[2] = inicializar_kernel_inotify(&inotify);
	rets[3] = l_thread_periodic_create(&metadata_updater,
			&actualizar_tablas_threaded, &get_refresh_metadata, NULL);
	rets[4] = l_thread_periodic_create(&memorias_updater,
			&actualizar_memorias_threaded, &get_refresh_memorias, NULL);

	for (int i = 0; i < 5; i++) {
		if (rets[i] != 0) {
			finalizar_thread_si_se_creo(&planificador, rets[0]);
			finalizar_thread_si_se_creo(&consola, rets[1]);
			finalizar_thread_si_se_creo(&inotify, rets[2]);
			finalizar_thread_si_se_creo(&metadata_updater.l_thread, rets[3]);
			finalizar_thread_si_se_creo(&memorias_updater.l_thread, rets[4]);
			liberar_recursos_kernel();
			exit(EXIT_FAILURE);
		}
	}

	kernel_log_to_level(LOG_LEVEL_INFO, "Todos los módulos inicializados.");

	pthread_mutex_lock(&kernel_main_mutex);
	pthread_cond_wait(&kernel_main_cond, &kernel_main_mutex);
	pthread_mutex_unlock(&kernel_main_mutex);

	finalizar_thread(&planificador);
	finalizar_thread(&consola);
	finalizar_thread(&inotify);
	finalizar_thread(&metadata_updater.l_thread);
	finalizar_thread(&memorias_updater.l_thread);
	kernel_log_to_level(LOG_LEVEL_INFO, "Finalizando.");
	liberar_recursos_kernel();
	return 0;

}
