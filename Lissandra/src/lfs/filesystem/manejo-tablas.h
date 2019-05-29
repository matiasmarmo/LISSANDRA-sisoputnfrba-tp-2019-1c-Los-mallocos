#ifndef LFS_FILE_SYSTEM_MANEJO_TABLAS_H_
#define LFS_FILE_SYSTEM_MANEJO_TABLAS_H_

#include <stdint.h>
#include <sys/stat.h>
#include <commons/collections/list.h>

typedef struct metadata {
	uint8_t consistencia;
	uint8_t n_particiones;
	uint32_t t_compactaciones;
} metadata_t;

int crear_tabla(char* nombre, metadata_t metadata);

int existe_tabla(char* nombre_tabla);

int obtener_metadata_tabla(char* nombre_tabla, metadata_t* metadata_tabla);

void obtener_path_particion(int numero, char* nombre_tabla, char* path);

int borrar_tabla(char *tabla);

void borrar_todos_los_tmpc(char *tabla);

void convertir_todos_tmp_a_tmpc(char* tabla);

int dar_metadata_tablas(t_list *nombre_tablas, t_list *metadatas);

void obtener_campos_metadatas(uint8_t* consistencias,
		uint8_t* numeros_particiones, uint32_t* tiempos_compactaciones,
		t_list* metadatas);

int crear_particion(int numero, char* nombre_tabla);

int crear_temporal(int numero, char* nombre_tabla);

int iterar_directorio_tabla(char *tabla,
		int (funcion)(const char*, const struct stat*, int));

#endif
