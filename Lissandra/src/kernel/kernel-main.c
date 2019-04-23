#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

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

int main() {
	inicializar_metricas();
	inicializar_kernel_config();
	lissandra_thread_t consola, planificador;
	l_thread_create(&planificador, &correr_planificador, NULL);
	l_thread_create(&consola, &correr_consola, NULL);

	pthread_mutex_lock(&kernel_main_mutex);
	pthread_cond_wait(&kernel_main_cond, &kernel_main_mutex);
	pthread_mutex_unlock(&kernel_main_mutex);

	l_thread_solicitar_finalizacion(&consola);
	l_thread_join(&consola, NULL);
	l_thread_solicitar_finalizacion(&planificador);
	l_thread_join(&planificador, NULL);
	destruir_kernel_config();
	destruir_metricas();
	return 0;

}
