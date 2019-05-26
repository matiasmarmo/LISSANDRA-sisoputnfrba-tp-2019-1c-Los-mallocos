#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <inttypes.h>
#include <commons/collections/list.h>
#include <commons/log.h>
#include <string.h>

#include "../commons/lissandra-threads.h"
#include "memoria-logger.h"
#include "memoria-config.h"
#include "memoria-server.h"
#include "memoria-main.h"

struct segmento{
	char* tabla;
	t_list* registro_base;
	int registro_limite;
};
struct registro_tabla_pagina{
	uint16_t numero_pagina;
	void* puntero_a_pagina; //MARCO
	uint8_t flag_modificado;
};
typedef struct pagina_t{
	uint64_t* timestamp;
	uint16_t* key;
	char* value;
}pagina;


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
void crear_segmento_nuevo(t_list* tabla_segmentos,char* nombre_tabla_nueva){
	struct segmento nuevo_segmento;
	nuevo_segmento.tabla = nombre_tabla_nueva;
	nuevo_segmento.registro_base = list_create();
	nuevo_segmento.registro_limite = 0;
	list_add(tabla_segmentos, &nuevo_segmento);
}
int encontrar_pagina_vacia(uint8_t* memoria,int tamanio_memoria,int tamanio_maximo_pagina){
	int numero_pagina = 0;
	//int paginas = tamanio_memoria / tamanio_maximo_pagina;
	while(tamanio_memoria >= tamanio_maximo_pagina + numero_pagina){
		if(	*(memoria + numero_pagina) == 0){
			return numero_pagina;
		}
		numero_pagina += tamanio_maximo_pagina;
	}
	return -1;
}

int main() {
	//inicializar_memoria();
	int tamanio_memoria = 45; //int tamanio_memoria = get_tamanio_memoria();
	int tamanio_maximo_value = 10; // PEDIRSELO AL FS
	uint8_t* memoria = calloc(tamanio_memoria, sizeof(char));

	// inserto 2 paginas para chequear si funciona OK
	pagina nueva_pagina;
	nueva_pagina.key = (uint16_t*)(memoria + 0);
	nueva_pagina.timestamp = (uint64_t*)(memoria + 2);
	nueva_pagina.value = (char*)(memoria + 10);
	*(nueva_pagina.key) = 10;
	*(nueva_pagina.timestamp) = 1;
	strcpy(nueva_pagina.value,"matias");

	pagina nueva_pagina2;
	nueva_pagina2.key = (uint16_t*)(memoria + 21);
	nueva_pagina2.timestamp = (uint64_t*)(memoria + 23);
	nueva_pagina2.value = (char*)(memoria + 31);
	*(nueva_pagina2.key)=3;
	*(nueva_pagina2.timestamp) = 1;
	strcpy(nueva_pagina2.value,"matias");

	int tamanio_maximo_pagina = sizeof(uint16_t) + sizeof(uint64_t) + tamanio_maximo_value + 1; //BYTES

	//memcpy(&memoria[0] , &nueva_pagina , tamanio_maximo_pagina );

	//memoria[0] = nueva_pagina.key;
	//memoria[2] = nueva_pagina.timestamp;
	//memoria[1] = nueva_pagina2;

	int lugar_pagina_vacia = encontrar_pagina_vacia(memoria,tamanio_memoria,tamanio_maximo_pagina);
	printf("lugar_pagina_vacia: %d",lugar_pagina_vacia);

	free(memoria);

	// t_list* TABLA_DE_SEGMENTOS = list_create();
	//	int tamanio_memoria = 1000;//get_tamanio_memoria();
		//struct pagina pagina;
	//	t_list* TABLA_DE_SEGMENTOS = list_create();
		//t_list* PAGINAS = list_create();
	//	struct segmento a;
	//	a.tabla = "tabla1";
	//	a.registro_base = NULL;
	//	a.registro_limite = NULL;
/*
		int j;
		j=list_size(TABLA_DE_SEGMENTOS);
		printf("inicial %d",j);
		//crear_segmento_nuevo(TABLA_DE_SEGMENTOS,"tabla1");
		//printf("%d     ",nuevo.registro_limite);
		//list_add(TABLA_DE_SEGMENTOS, &a);
		j=list_size(TABLA_DE_SEGMENTOS);
		printf("final %d",j);*/
	//char* memoria = calloc(tamanio_memoria, sizeof(char));

	/*inicializar_memoria_logger();
	inicializar_memoria_config();
	lissandra_thread_t l_thread;
	l_thread_create(&l_thread, &correr_servidor_memoria, NULL);
	sleep(1000000);
	l_thread_solicitar_finalizacion(&l_thread);
	l_thread_join(&l_thread, NULL);
	destruir_memoria_logger();
	destruir_memoria_config();*/

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




