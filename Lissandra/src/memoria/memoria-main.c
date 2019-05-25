#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>

#include "../commons/lissandra-threads.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

struct segmento{
	char* tabla;
	void* registro_base;
	void* registro_limite;
};
struct registro_tabla_pagina{
	uint16_t numero_pagina;
	void* puntero_a_pagina; //MARCO
	uint8_t flag_modificado;
};
struct pagina{
	uint64_t timestamp;
	uint16_t key;
	char* value;
};


//pthread_mutex_t memoria_main_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t memoria_main_cond = PTHREAD_COND_INITIALIZER;


void inicializar_memoria() {

}
/*
void liberar_recursos_memoria() {

}
*/

void finalizar_memoria(){

}

int main() {
	//inicializar_memoria();
	int tamanio_memoria = get_tamanio_memoria();
	char* memoria = calloc(tamanio_memoria, sizeof(char));

	t_list* TABLA_DE_SEGMENTOS = list_create();
	t_list* TABLA_DE_PAGINAS = list_create();

	//	int tamanio_memoria = 1000;//get_tamanio_memoria();
		//struct pagina pagina;
	//	t_list* TABLA_DE_SEGMENTOS = list_create();
		//t_list* PAGINAS = list_create();
	//	struct segmento a;
	//	a.tabla = "tabla1";
	//	a.registro_base = NULL;
	//	a.registro_limite = NULL;

		int j;
		j=list_size(TABLA_DE_SEGMENTOS);
		printf("inicial %d",j);
		crear_segmento_nuevo(TABLA_DE_SEGMENTOS,"tabla1",5);
		//list_add(TABLA_DE_SEGMENTOS, &a);
		j=list_size(TABLA_DE_SEGMENTOS);
		printf("final %d",j);
	//char* memoria = calloc(tamanio_memoria, sizeof(char));

	t_list* TABLA_DE_SEGMENTOS = list_create();
	t_list* TABLA_DE_PAGINAS = list_create();

	inicializar_memoria_logger();
	inicializar_memoria_config();
	lissandra_thread_t l_thread;
	l_thread_create(&l_thread, &correr_servidor_memoria, NULL);
	sleep(1000000);
	l_thread_solicitar_finalizacion(&l_thread);
	l_thread_join(&l_thread, NULL);
	destruir_memoria_logger();
	destruir_memoria_config();
	return 0;

	// inicializo hilos

	//pthread_mutex_lock(&memoria_main_mutex);
	//pthread_cond_wait(&memoria_main_cond, &memoria_main_mutex);
	//pthread_mutex_unlock(&memoria_main_mutex);

	// finalizar hilos

	//liberar_recursos_memoria();
	return 0;
}

void crear_todas_las_paginas_del_segmento_vacias(t_list* tabla_paginas,int cantidad_paginas){
	struct registro_tabla_pagina nuevo_registro_pagina;
	nuevo_registro_pagina.numero_pagina=0;
	nuevo_registro_pagina.puntero_a_pagina=NULL;
	nuevo_registro_pagina.flag_modificado=0;
	for(int i=0;i<cantidad_paginas;i++){
		nuevo_registro_pagina.numero_pagina=i;
		list_add(tabla_paginas, &nuevo_registro_pagina);
	}
}
void crear_segmento_nuevo(t_list* tabla_segmentos,t_list* tabla_paginas,char* nombre_tabla_nueva,int tamanio){
	int inicio_segmento_en_tabla_paginas = list_size(tabla_paginas) + 1;
	struct segmento nuevo_segmento;
	nuevo_segmento.tabla = nombre_tabla_nueva;
	nuevo_segmento.registro_base = inicio_segmento_en_tabla_paginas;
	nuevo_segmento.registro_limite = tamanio;
	list_add(tabla_segmentos, &nuevo_segmento);
}



