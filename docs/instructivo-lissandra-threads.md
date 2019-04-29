# Lissandra threads

El módulo lissandra threads expone estructuras wrappers de pthread_t para implementar funcionalidad extra al trabajar con hilos. 
Las mismas se usan como reemplazos de pthread_t y permiten:

* Que el hilo hijo indique que finalizó su ejecución
* Que el hilo padre consulte si el hijo indicó que finalizó su ejecución
* Que el hilo padre solicite que el hijo finalice su ejecución
* Que el hilo hijo consulte si el padre le solicitó finalizar su ejecución
* Instanciar un hilo que ejecuta una función cada cierto intervalo  de tiempo, el cual puede variar después de instanciado el hilo (caso de uso: ejecutar una función cada cierto tiempo definido en un archivo de configuración)


### La estructura `lissandra_thread_t`

Esta es el wrapper base del módulo. Su definición es la siguiente:

``` C
typedef struct lissandra_thread {
	pthread_t thread;
	pthread_mutex_t lock;
	bool debe_finalizar;
	bool finalizada;
	void *entrada;
} lissandra_thread_t;
```

Como puede verse posee: 
* Un `pthread_t` en el cual se instanciará el hilo
* Un `pthread_mutex_t` para sincronización
* Un flag que indica si el hilo debe finalizar
* Otro flag que indica si el hilo finalizó
* Un `void*` que podrá ser utilizado para pasar información a modo de parámetro a la función a ejecutar (similar al que recibe un hilo instanciado directamente con `pthread_t`)

### Instanciar un lissandra thread

Las lissandra threads se instancian utilizando la siguiente función:

``` C
int l_thread_create(lissandra_thread_t *l_thread, lissandra_thread_func funcion,
		void* entrada);
```

La misma recibe un puntero al `lissandra_thread_t` a utilizar, la función a ejecutar (con la siguiente firma: `void *f(void *)`; igual a la que se le provee a `pthread_create`) y el `void *` que se asignará al campo entrada de la estructura.
Cabe aclarar que la función a ejecutar no recibirá como parámetro el `void *` especificado como entrada; sino que recibirá un puntero a su `lissandra_thread_t`, pudiendo acceder a la entrada a través de esta.
La función retorna `0` en caso de éxito, `-1` en caso de error.

### Solicitar la finalización de un lissandra_thread

Para solicitar la finalización se utiliza:

``` C
int l_thread_solicitar_finalizacion(lissandra_thread_t *l_thread);
```

La función recibe un puntero al `lissandra_thread_t` y le indica el hilo que finalice.
Retorna `0` en caso de éxito, `-1` en caso de error.

### Consultar si se solicitó la finalización

Para consultar si se solicitó la finalización se utiliza:

``` C
int l_thread_debe_finalizar(lissandra_thread_t *l_thread);
```

Recibe un puntero al `lissandra_thread_t` y retorna `-1` en caso de error, `0` si no se solicitó la finalización o `1` si se solicitó la finalización.

### Indicar la finalización de un lissandra_thread

Para indicar la finalización se utiliza:

``` C
int l_thread_indicar_finalizacion(lissandra_thread_t *l_thread);
```

La función recibe un puntero al `lissandra_thread_t` y e indica que este ya finalizó.
Retorna `0` en caso de éxito, `-1` en caso de error.

### Consultar si se indicó la finalización

Para consultar si se indicó la finalización se utiliza:

``` C
int l_thread_finalizo(lissandra_thread_t *l_thread);
```

Recibe un puntero al `lissandra_thread_t` y retorna `-1` en caso de error, `0` si no se indicó la finalización o `1` si se indicó la finalización.

### Joinear un lissandra thread

Al igual que con un `pthread_t`; realizar un join a un lissandra thread espera a que el mismo finalice y, luego, libera sus recursos.
Para este fin se usa:

``` C
int l_thread_join(lissandra_thread_t *l_thread, void **resultado);
```

El parámetro resultado es el mismo que se le daría a `pthread_join`. Es el puntero al que se asignará el valor retornado por la funcíón ejecutada en el hilo.

### Ejemplo

``` C
#include <stdio.h>
#include <unistd.h>
#include "lissandra-threads.h"

void *funcion(void *input) {
    lissandra_thread_t *l_thread = (lissandra_thread_t*) input;
    void *entrada_hilo = l_thread->entrada;
    while(!l_thread_debe_finalizar(l_thread)) {
        printf("Hilo\n");
        sleep(1);
    }
    l_thread_indicar_finalizacion(l_thread);
    pthread_exit(NULL);
}

int main() {
    lissandra_thread_t l_thread;
    if(l_thread_create(&l_thread, &funcion, NULL) < 0) {
        printf("Error\ņ");
        return -1;
    }
    printf("Hilo inicializado\n");
    sleep(10);
    l_thread_solicitar_finalizacion(&l_thread);
    l_thread_join(&l_thread, NULL);
    printf("Hilo finalizado\n");
    return 0;
}
```

### Ejecutar una función de forma periódica

Para ejecutar una función de forma periódica, el módulo expone un segundo wrapper:

``` C
typedef struct lissandra_thread_periodic {
	lissandra_thread_t l_thread;
	lissandra_thread_func funcion_periodica;
	interval_getter_t interval_getter;
} lissandra_thread_periodic_t;
```

La función `interval_getter` tiene la firma `int f()`. La misma es una función que retorna el tiempo que debe pasar entre ejecuciones de la función periódica, expresado en milisegundos.

### Instanciar un lissandra thread periódico

Se realiza con la función:

``` C
int l_thread_periodic_create(lissandra_thread_periodic_t *lp_thread,
		lissandra_thread_func funcion, interval_getter_t interval_getter,
		void* entrada);
```

Recibe un puntero a la estructura `lissandra_thread_periodic_t` a utilizar, la función a ejecutar, la función que retorna el intervalo y el `void*` de entrada a la función.

Notar que la función que se ejecuta sigue recibiendo un puntero al `lissandra_thread_t` y no al `lissandra_thread_periodic_t`

### Obtener el intervalo

Se realiza con 

``` C
uint32_t l_thread_periodic_get_intervalo(lissandra_thread_periodic_t *lp_thread);
```

La función recibe un puntero al `lissandra_thread_periodic_t` y retorna el intervalo.

### Modificar el getter del intervalo en tiempo de ejecución del hilo

También se permite modificar la función con:

``` C
int l_thread_periodic_set_interval_getter(
		lissandra_thread_periodic_t *lp_thread,
		interval_getter_t interval_getter);
```

La función recibe un puntero al `lissandra_thread_periodic_t` y la nueva función a utilizar. Retorna `0` en caso de éxito y `-1` en caso de error.


