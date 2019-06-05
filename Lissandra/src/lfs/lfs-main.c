#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "../commons/lissandra-threads.h"
#include "filesystem/filesystem.h"
#include "lissandra/memtable.h"
#include "lissandra/dump.h"
#include "compactador.h"
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
    if(inicializar_filesystem() < 0) {
        lfs_log_to_level(LOG_LEVEL_ERROR, false,
                "Error al inicializar filesystem. Abortando.");
        destruir_lfs_logger();
        destruir_lfs_config();
        exit(EXIT_FAILURE);   
    }
    if(inicializar_memtable() < 0) {
        lfs_log_to_level(LOG_LEVEL_ERROR, false,
                "Error al inicializar la memtable. Abortando.");
        destruir_lfs_logger();
        destruir_lfs_config();
        exit(EXIT_FAILURE);   
    }
    if(inicializar_dumper() < 0) {
        lfs_log_to_level(LOG_LEVEL_ERROR, false,
                "Error al inicializar el dumper. Abortando.");
        destruir_lfs_logger();
        destruir_lfs_config();
        destruir_memtable();
        exit(EXIT_FAILURE);   
    }
    if(inicializar_compactador() < 0) {
        lfs_log_to_level(LOG_LEVEL_ERROR, false,
                "Error al inicializar el compactador. Abortando.");
        destruir_lfs_logger();
        destruir_lfs_config();
        destruir_memtable();
        destruir_dumper();
        exit(EXIT_FAILURE);   
    }
}

void liberar_recursos_lfs() {
	destruir_lfs_config();
	destruir_lfs_logger();
    destruir_compactador();
    destruir_dumper();
    destruir_memtable();
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
	int rets[3];
	inicializar_lfs();
	lissandra_thread_t servidor, inotify;
    lissandra_thread_periodic_t dumper;

	rets[0] = l_thread_create(&servidor, &correr_servidor, NULL);
	rets[1] = inicializar_lfs_inotify(&inotify);
    rets[2] = instanciar_hilo_dumper(&dumper);

	for (int i = 0; i < 3; i++) {
		if (rets[i] != 0) {
			finalizar_thread_si_se_creo(&servidor, rets[0]);
			finalizar_thread_si_se_creo(&inotify, rets[1]);
            finalizar_thread_si_se_creo(&(dumper.l_thread), rets[2]);
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
    finalizar_thread(&(dumper.l_thread));
    dumpear(NULL);
    compactar_todas_las_tablas();
	lfs_log_to_level(LOG_LEVEL_INFO, false, "Finalizando.");
	liberar_recursos_lfs();
	return 0;

}
