#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../../commons/comunicacion/protocol.h"
#include "../lfs-config.h"
#include "manejo-tablas.h"
#include "filesystem.h"

void campo_entero_a_string(int entero, char* string) {
	sprintf(string, "%d", entero);
}

void obtener_path_directorio_tablas(char *path) {
	char temporal[TAMANIO_PATH];
	strcat(temporal, get_punto_montaje());
	strcat(temporal, NOMBRE_DIRECTORIO_TABLAS);
	strcpy(path, temporal);
}

int crear_directorio_tabla(char* nombre_tabla) {
	int res_mkdir;

	if ((res_mkdir = mkdir(nombre_tabla,
	S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
			== -1) {return -1;}
	return 0;
}

int crear_metadata_tabla(metadata_t metadata, char* nombre_tabla) {
	char escritura[200];
	char consistencia_string[10];
	char n_particiones_string[15];
	char t_compactaciones_string[15];
	char nombre_metadata[TAMANIO_NOMBRE_ARCHIVO];
	FILE* metadata_tabla_f;

	strcpy(nombre_metadata, nombre_tabla);
	strcat(nombre_metadata, "-metadata");
	strcat(nombre_metadata, ".bin");
	if ((metadata_tabla_f = fopen(nombre_metadata, "wb")) == NULL) {return -1;}

	strcpy(escritura, "CONSISTENCY=");
	campo_entero_a_string(metadata.consistencia, consistencia_string);
	strcat(escritura, consistencia_string);
	strcat(escritura, "\nPARTITIONS=");
	campo_entero_a_string(metadata.n_particiones, n_particiones_string);
	strcat(escritura, n_particiones_string);
	strcat(escritura, "\nCOMPACTION_TIME=");
	campo_entero_a_string(metadata.t_compactaciones, t_compactaciones_string);
	strcat(escritura, t_compactaciones_string);

	fwrite(escritura, strlen(escritura), 1, metadata_tabla_f);

	fclose(metadata_tabla_f);
	return 0;
}

int crear_particion(int numero, char* nombre_tabla, int tamanio) {
	FILE* binario_f;
	char nombre_binario[TAMANIO_NOMBRE_ARCHIVO], escritura[200];
	char numero_string[10], size_string[15], block_string[10];
	int bloque;

	strcpy(nombre_binario, nombre_tabla);
	strcat(nombre_binario, "-");
	campo_entero_a_string(numero, numero_string);
	strcat(nombre_binario, numero_string);
	strcat(nombre_binario, ".bin");

	if ((binario_f = fopen(nombre_binario, "wb")) == NULL) {
		return -1;
	}

	strcpy(escritura, "SIZE=");
	campo_entero_a_string(tamanio, size_string);
	strcat(escritura, size_string);

	strcat(escritura, "\nBLOCKS=[");
	bloque = numero; // Cuando este implementada, hay que usar la funcion pedir_bloque() en lugar de numero
	campo_entero_a_string(bloque, block_string);
	strcat(escritura, block_string);
	strcat(escritura, "]");

	fwrite(escritura, strlen(escritura), 1, binario_f);
	fclose(binario_f);

	return 0;
}

int crear_particiones(int n_particiones, char* nombre_tabla) {
	int tamanio_binario = BLOCK_SIZE; // leer comentario del #define de block_size del .h

	for(int numero = 0 ; numero < n_particiones ; numero++){
		crear_particion(numero, nombre_tabla, tamanio_binario);
	}

	return 0;
}

int crear_tabla(char* nombre_tabla, metadata_t metadata) {
	int res_tabla;
	char path_actual[TAMANIO_PATH], path[TAMANIO_PATH];
	int res_chdir;
	char* res_cwd;

	if ((res_cwd = getcwd(path_actual, sizeof(path_actual))) == NULL) {return -1;}
	obtener_path_directorio_tablas(path);

	if ((res_chdir = chdir(path)) == -1) {return -1;}
	if ((res_tabla = crear_directorio_tabla(nombre_tabla)) == -1) {return -1;}

	if ((res_chdir = chdir(nombre_tabla)) == -1) {return -1;}
	if ((res_tabla = crear_metadata_tabla(metadata, nombre_tabla)) == -1) {return -1;}
	if ((res_tabla = crear_particiones(metadata.n_particiones, nombre_tabla)) == -1) {return -1;}

	chdir(path_actual);
	return 0;
}
