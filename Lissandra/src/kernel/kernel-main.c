#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <commons/collections/list.h>

#include "../commons/lissandra-threads.h"
#include "pool-memorias/manager-memorias.h"
#include "pool-memorias/metadata-tablas.h"
#include "ejecucion/planificador.h"
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
	if (inicializar_kernel_config() < 0) {
		destruir_metricas();
		exit(EXIT_FAILURE);
	}
	if (inicializar_memorias() < 0) {
		printf("Error init memorias\n");
		destruir_metricas();
		destruir_kernel_config();
		exit(EXIT_FAILURE);
	}
}

void liberar_recursos_kernel() {
	destruir_metricas();
	destruir_kernel_config();
	destruir_memorias();
}

void finalizar_thread(lissandra_thread_t *l_thread) {
	l_thread_solicitar_finalizacion(l_thread);
	l_thread_join(l_thread, NULL);
}

int main() {
	inicializar_kernel();
	int planificador_ret, consola_ret, inotify_ret;
	lissandra_thread_t planificador, consola, inotify;
	planificador_ret = l_thread_create(&planificador, &correr_planificador,
			NULL);
	consola_ret = l_thread_create(&consola, &correr_consola, NULL);
	inotify_ret = inicializar_kernel_inotify(&inotify);

	if (planificador_ret != 0 || consola_ret != 0 || inotify_ret != 0) {
		printf("Error threads\n");
		finalizar_thread(&planificador);
		finalizar_thread(&consola);
		finalizar_thread(&inotify);
		liberar_recursos_kernel();
		exit(EXIT_FAILURE);
	}

	printf("Listo\n");
	pthread_mutex_lock(&kernel_main_mutex);
	pthread_cond_wait(&kernel_main_cond, &kernel_main_mutex);
	pthread_mutex_unlock(&kernel_main_mutex);

	finalizar_thread(&planificador);
	finalizar_thread(&consola);
	finalizar_thread(&inotify);
	liberar_recursos_kernel();
	return 0;

}
