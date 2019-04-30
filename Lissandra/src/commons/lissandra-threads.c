#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <signal.h>

#include "lissandra-threads.h"

int l_thread_init(lissandra_thread_t *l_thread) {
	int error;
	if ((error = pthread_mutex_init(&l_thread->lock, NULL)) != 0) {
		return -error;
	}
	l_thread->debe_finalizar = false;
	l_thread->finalizada = false;
	return 0;
}

int l_thread_start(lissandra_thread_t *l_thread, lissandra_thread_func funcion,
		void *entrada) {
	int error;
	l_thread->entrada = entrada;
	if ((error = pthread_create(&l_thread->thread, NULL, funcion, l_thread))
			!= 0) {
		pthread_mutex_destroy(&l_thread->lock);
		return -error;
	}
	return 0;
}

void l_thread_destroy(lissandra_thread_t *l_thread) {
	pthread_mutex_destroy(&l_thread->lock);
}

int l_thread_create(lissandra_thread_t *l_thread, lissandra_thread_func funcion,
		void* entrada) {
	int error;
	if ((error = l_thread_init(l_thread)) != 0) {
		return -error;
	}
	if ((error = l_thread_start(l_thread, funcion, entrada)) != 0) {
		return -error;
	}
	return 0;
}

int l_thread_join(lissandra_thread_t *l_thread, void** resultado) {
	int join_res = pthread_join(l_thread->thread, resultado);
	if (join_res == 0) {
		l_thread_destroy(l_thread);
	}
	return join_res;
}

int obtener_valor(lissandra_thread_t *l_thread,
		int (*getter)(lissandra_thread_t*)) {
	int error, resultado;
	if ((error = pthread_mutex_lock(&l_thread->lock)) != 0) {
		return -error;
	}
	resultado = getter(l_thread);
	// El mutex acaba de ser bloqueado exitosamente y no se utilizarán locks recursivos,
	// por lo que pthread_mutex_unlock no va a fallar y podemos prescindir de manejar
	// el posible error.
	// Para más información: https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_lock.html
	pthread_mutex_unlock(&l_thread->lock);
	return resultado;
}

int obtener_finalizada(lissandra_thread_t *l_thread) {
	return l_thread->finalizada;
}

int obtener_debe_finalizar(lissandra_thread_t *l_thread) {
	return l_thread->debe_finalizar;
}

int l_thread_finalizo(lissandra_thread_t *l_thread) {
	return obtener_valor(l_thread, &obtener_finalizada);
}

int l_thread_debe_finalizar(lissandra_thread_t *l_thread) {
	return obtener_valor(l_thread, &obtener_debe_finalizar);
}

int setear_valor(lissandra_thread_t *l_thread,
		void (*setter)(lissandra_thread_t*)) {
	int error;
	if ((error = pthread_mutex_lock(&l_thread->lock)) != 0) {
		return -error;
	}
	setter(l_thread);
	// El mutex acaba de ser bloqueado exitosamente y no se utilizarán locks recursivos,
	// por lo que pthread_mutex_unlock no va a fallar y podemos prescindir de manejar
	// el posible error.
	// Para más información: https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_lock.html
	pthread_mutex_unlock(&l_thread->lock);
	return 0;
}

void setear_finalizada(lissandra_thread_t *l_thread) {
	l_thread->finalizada = true;
}

void setear_debe_finalizar(lissandra_thread_t *l_thread) {
	l_thread->debe_finalizar = true;
}

int l_thread_indicar_finalizacion(lissandra_thread_t *l_thread) {
	return setear_valor(l_thread, &setear_finalizada);
}

int l_thread_solicitar_finalizacion(lissandra_thread_t *l_thread) {
	return setear_valor(l_thread, &setear_debe_finalizar);
}

uint32_t l_thread_periodic_get_intervalo(lissandra_thread_periodic_t *lp_thread) {
	int error;
	if ((error = pthread_mutex_lock(&lp_thread->l_thread.lock)) != 0) {
		return -error;
	}
	uint32_t resultado = lp_thread->interval_getter();
	pthread_mutex_unlock(&lp_thread->l_thread.lock);
	return resultado;
}

int l_thread_periodic_set_interval_getter(
		lissandra_thread_periodic_t *lp_thread,
		interval_getter_t interval_getter) {
	int error;
	if ((error = pthread_mutex_lock(&lp_thread->l_thread.lock)) != 0) {
		return -error;
	}
	lp_thread->interval_getter = interval_getter;
	pthread_mutex_unlock(&lp_thread->l_thread.lock);
	return 0;
}

void *wrapper_periodica(void* entrada) {
	lissandra_thread_periodic_t *lp_thread =
			(lissandra_thread_periodic_t*) entrada;
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its, read_its;
	uint32_t intervalo;

	if ((intervalo = l_thread_periodic_get_intervalo(lp_thread)) < 0) {
		l_thread_indicar_finalizacion(&lp_thread->l_thread);
		pthread_exit(NULL);
	}

	memset(&sev, 0, sizeof(struct sigevent));
	sev.sigev_notify = SIGEV_NONE;
	sev.sigev_signo = 0;
	sev.sigev_value.sival_int = 0;
	its.it_value.tv_sec = intervalo / 1000;
	its.it_value.tv_nsec = (intervalo % 1000) * 1000000;
	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
		l_thread_indicar_finalizacion(&lp_thread->l_thread);
		pthread_exit(NULL);
	}

	if (timer_settime(timerid, 0, &its, NULL) == -1) {
		timer_delete(timerid);
		l_thread_indicar_finalizacion(&lp_thread->l_thread);
		pthread_exit(NULL);
	}

	while (!l_thread_debe_finalizar(&lp_thread->l_thread)) {
		if (timer_gettime(timerid, &read_its) != 0) {
			break;
		}
		if (read_its.it_value.tv_sec == 0 && read_its.it_value.tv_nsec == 0) {
			lp_thread->funcion_periodica(&lp_thread->l_thread);
			if ((intervalo = l_thread_periodic_get_intervalo(lp_thread)) < 0) {
				break;
			}
			its.it_value.tv_sec = intervalo / 1000;
			its.it_value.tv_nsec = (intervalo % 1000) * 1000000;
			if (timer_settime(timerid, 0, &its, NULL) == -1) {
				break;
			}
		}
		usleep(200000);
	}

	timer_delete(timerid);
	l_thread_indicar_finalizacion(&lp_thread->l_thread);
	pthread_exit(NULL);
}

int l_thread_periodic_create(lissandra_thread_periodic_t *lp_thread,
		lissandra_thread_func funcion, interval_getter_t interval_getter,
		void* entrada) {
	int error;
	lp_thread->funcion_periodica = funcion;
	lp_thread->interval_getter = interval_getter;
	if((error = l_thread_init(&lp_thread->l_thread)) != 0) {
		return error;
	}
	lp_thread->l_thread.entrada = entrada;
	if((error = pthread_create(&lp_thread->l_thread.thread, NULL, &wrapper_periodica, lp_thread)) != 0) {
		l_thread_destroy(&lp_thread->l_thread);
		return -error;
	}
	return 0;
}
