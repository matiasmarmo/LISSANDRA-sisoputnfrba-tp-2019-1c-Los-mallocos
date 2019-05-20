#ifndef LFS_FILE_SYSTEM_MANEJO_TABLAS_H_
#define LFS_FILE_SYSTEM_MANEJO_TABLAS_H_

#include <stdint.h>

#define BLOCK_SIZE 64 	// El tamanio de cada bloque debe ser leido del archivo
						// metadata.bin de la carpeta metadata mediante alguna funcion.
						// Se definio aca porque todavia no existe dicha funcion

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

#endif
