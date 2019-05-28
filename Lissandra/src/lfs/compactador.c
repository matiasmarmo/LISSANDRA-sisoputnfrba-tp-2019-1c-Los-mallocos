#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <commons/collections/dictionary.h>

#include "../commons/lissandra-threads.h"
#include "filesystem/manejo-tablas.h"

pthread_mutex_t hilos_compactadores_mutex = PTHREAD_MUTEX_INITIALIZER;
t_dictionary *hilos_compactadores;

int inicializar_compactador() {
    if(pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
        return -1;
    }
    hilos_compactadores = dictionary_create();
    pthread_mutex_unlock(&hilos_compactadores_mutex);
    return hilos_compactadores == NULL ? -1 : 0;
}

void destruir_hilo_compactador(void *hilo) {
    lissandra_thread_periodic_t *lp_thread = (lissandra_thread_periodic_t*) hilo;
    l_thread_solicitar_finalizacion(&lp_thread->l_thread);
    l_thread_join(&lp_thread->l_thread, NULL);
    free(lp_thread->l_thread.entrada);
    free(hilo);
}

int destruir_compactador() {
    if(pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
        return -1;
    }
    dictionary_destroy_and_destroy_elements(hilos_compactadores, &destruir_hilo_compactador);
    pthread_mutex_unlock(&hilos_compactadores_mutex);
    return 0;
}

void *compactar(void *data) {
    lissandra_thread_t *l_thread = (lissandra_thread_t*) data;
    char *nombre_tabla = (char*) l_thread->entrada;
    return NULL;
}

int instanciar_hilo_compactador(char *tabla, int t_compactaciones) {
    lissandra_thread_periodic_t *lp_thread;
    char *entrada_thread;
    if(pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
        return -1;
    }
    if(dictionary_has_key(hilos_compactadores, tabla)) {
        pthread_mutex_unlock(&hilos_compactadores_mutex);
        return -1;
    }
    entrada_thread = strdup(tabla);
    if(entrada_thread == NULL) {
        pthread_mutex_unlock(&hilos_compactadores_mutex);
        return -1;
    }
    lp_thread = malloc(sizeof(lissandra_thread_periodic_t));
    if(lp_thread == NULL) {
        pthread_mutex_unlock(&hilos_compactadores_mutex);
        free(entrada_thread);
        return -1;
    }
    if(l_thread_periodic_create_fixed(lp_thread,
            &compactar, t_compactaciones,
            entrada_thread) < 0) {
        pthread_mutex_unlock(&hilos_compactadores_mutex);
        free(lp_thread);
        free(entrada_thread);
        return -1;
    }
    dictionary_put(hilos_compactadores, tabla, lp_thread);
    pthread_mutex_unlock(&hilos_compactadores_mutex);
    return 0;
}

int finalizar_hilo_compactador(char *tabla) {
    if(pthread_mutex_lock(&hilos_compactadores_mutex) != 0) {
        return -1;
    }
    if(!dictionary_has_key(hilos_compactadores, tabla)) {
        pthread_mutex_unlock(&hilos_compactadores_mutex);
        return -1;
    }
    void *hilo_compactador = dictionary_remove(hilos_compactadores, tabla);
    destruir_hilo_compactador(hilo_compactador);
    pthread_mutex_unlock(&hilos_compactadores_mutex);
    return 0;
}