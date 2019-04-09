#ifndef LISSANDRA_THREADS_H_
#define LISSANDRA_THREADS_H_

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef void *(*lissandra_thread_func)(void*);

typedef struct lissandra_thread {
	pthread_t thread;
	pthread_mutex_t lock;
	bool debe_finalizar;
	bool finalizada;
	void *entrada;
} lissandra_thread_t;

typedef struct lissandra_thread_periodic {
	lissandra_thread_t l_thread;
	lissandra_thread_func funcion_periodica;
	uint32_t intervalo;
} lissandra_thread_periodic_t;

int l_thread_create(lissandra_thread_t *l_thread, lissandra_thread_func funcion,
		void* entrada);
int l_thread_join(lissandra_thread_t *l_thread, void **resultado);

int l_thread_finalizo(lissandra_thread_t *l_thread);
int l_thread_debe_finalizar(lissandra_thread_t *l_thread);
int l_thread_solicitar_finalizacion(lissandra_thread_t *l_thread);
int l_thread_indicar_finalizacion(lissandra_thread_t *l_thread);

int l_thread_periodic_create(lissandra_thread_periodic_t *lp_thread,
		lissandra_thread_func funcion, uint32_t intervalo, void* entrada);
int l_thread_periodic_set_intervalo(lissandra_thread_periodic_t *lp_thread,
		uint32_t intervalo);
uint32_t l_thread_periodic_get_intervalo(lissandra_thread_periodic_t *lp_thread);

#endif
