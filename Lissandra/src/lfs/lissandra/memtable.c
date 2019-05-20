#include <pthread.h>
#include <stdlib.h>
#include <commons/collections/dictionary.h>

#include "../filesystem/filesystem.h"
#include "memtable.h"

pthread_rwlock_t memtable_lock = PTHREAD_RWLOCK_INITIALIZER;
t_dictionary *memtable;

typedef struct entrada_memtable {
    pthread_mutex_t lock;
    registro_t *data;
    int cantidad_registros;
    int tamanio_actual_data;
} entrada_memtable_t;

int inicializar_memtable() {
    if(pthread_rwlock_wrlock(&memtable_lock) != 0) {
        return -1;
    }
    memtable = dictionary_create();
    pthread_rwlock_unlock(&memtable_lock);
    return 0;
}

void destruir_memtable() {

    void _destruir_entrada(void *elemento) {
        entrada_memtable_t *entrada = (entrada_memtable_t*) elemento;
        free(entrada->data);
        free(entrada);
    }

    dictionary_destroy_and_destroy_elements(memtable, &_destruir_entrada);
}

entrada_memtable_t *construir_nueva_entrada() {
    entrada_memtable_t *nueva_entrada = malloc(sizeof(entrada_memtable_t));
    if(nueva_entrada == NULL) {
        return NULL;
    }
    if(pthread_mutex_init(&nueva_entrada->lock, NULL) != 0) {
        free(nueva_entrada);
        return NULL;
    }
    nueva_entrada->data = malloc(10 * sizeof(registro_t));
    if(nueva_entrada->data == NULL) {
        free(nueva_entrada);
        pthread_mutex_destroy(&nueva_entrada->lock);
        return NULL;
    }
    nueva_entrada->cantidad_registros = 0;
    nueva_entrada->tamanio_actual_data = 10;
    return nueva_entrada;
}

int insertar_en_entrada(entrada_memtable_t *entrada, registro_t registro) {
    if(pthread_mutex_lock(&entrada->lock) != 0) {
        return -1;
    }
    if(entrada->cantidad_registros >= entrada->tamanio_actual_data) {
        entrada->tamanio_actual_data = entrada->data == NULL ? 10 : entrada->tamanio_actual_data * 2;
        registro_t *data_realocado = realloc(entrada->data, entrada->tamanio_actual_data);
        if(data_realocado == NULL) {
            pthread_mutex_unlock(&entrada->lock);
            return -1;
        }
        entrada->data = data_realocado;
    }
    entrada->data[(entrada->cantidad_registros)++] = registro;
    pthread_mutex_unlock(&entrada->lock);
    return 0;
}


int insertar_en_memtable(registro_t registro, char *tabla) {
    if(pthread_rwlock_rdlock(&memtable_lock) != 0) {
        return -1;
    }
    if(dictionary_has_key(memtable, tabla)) {
        entrada_memtable_t *entrada = dictionary_get(memtable, tabla);
        pthread_rwlock_unlock(&memtable_lock);
        return insertar_en_entrada(entrada, registro);
    }
    pthread_rwlock_unlock(&memtable_lock);
    entrada_memtable_t *nueva_entrada = construir_nueva_entrada();
    if(nueva_entrada == NULL) {
        return -1;
    }
    if(pthread_rwlock_wrlock(&memtable_lock) != 0) {
        free(nueva_entrada->data);
        free(nueva_entrada);
        return -1;
    }
    if(insertar_en_entrada(nueva_entrada, registro) < 0) {
        free(nueva_entrada->data);
        free(nueva_entrada);
        pthread_rwlock_unlock(&memtable_lock);
        return -1;
    }
    dictionary_put(memtable, tabla, nueva_entrada);
    pthread_rwlock_unlock(&memtable_lock);
    return 0;
}

t_dictionary *obtener_datos_para_dumpear() {
    t_dictionary *resultado = dictionary_create();

    void _copiar_entrada(char *tabla, void *elemento) {
        entrada_memtable_t *entrada = (entrada_memtable_t*) elemento;
        memtable_data_t *res_data = malloc(sizeof(memtable_data_t));
        if(res_data == NULL) {
            // Si falla el malloc, se mantienen los datos de esa tabla
            // en la memtable hasta el proximo dump
            return;
        }
        res_data->data = entrada->data;
        res_data->cantidad_registros = entrada->cantidad_registros;
        dictionary_put(resultado, tabla, res_data);
        pthread_mutex_destroy(&entrada->lock);
    }

    void _liberar_entrada_memtable(void *entrada) {
        free(entrada);
    }

    if(pthread_rwlock_wrlock(&memtable_lock) != 0) {
        dictionary_destroy(resultado);
        return NULL;
    }
    dictionary_iterator(memtable, &_copiar_entrada);
    dictionary_clean_and_destroy_elements(memtable, &_liberar_entrada_memtable);
    pthread_rwlock_unlock(&memtable_lock);
    return resultado;
}