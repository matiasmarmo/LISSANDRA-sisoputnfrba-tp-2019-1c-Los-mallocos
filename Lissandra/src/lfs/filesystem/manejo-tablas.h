#ifndef LFS_FILE_SYSTEM_MANEJO_TABLAS_H_
#define LFS_FILE_SYSTEM_MANEJO_TABLAS_H_

#include <stdint.h>
#include <commons/collections/list.h>

typedef struct metadata {
	uint8_t consistencia;
	uint8_t n_particiones;
	uint32_t t_compactaciones;
} metadata_t;

int crear_tabla(char* nombre, metadata_t metadata);
/*
 * Crea el directorio para dicha tabla,
 * Crea el archivo Metadata asociado al mismo,
 * Graba en dicho archivo los campos de la estructura metadata_t pasada por argumento,
 * Crea los archivos binarios asociados a cada particion de la tabla y asigna a cada uno un bloque.
 */

int existe_tabla(char* nombre_tabla);

int obtener_metadata_tabla(char* nombre_tabla, metadata_t* metadata_tabla);

void obtener_path_particion(int numero, char* nombre_tabla, char* path);

int borrar_tabla(char *tabla);

void borrar_todos_los_tmpc(char *tabla);

void convertir_todos_tmp_a_tmpc(char* tabla);

int dar_metadata_tablas(t_list *nombre_tablas, t_list *metadatas);

#endif
