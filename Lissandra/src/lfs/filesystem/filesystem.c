#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <inttypes.h>
#include <time.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>
#include <commons/config.h>
#include <commons/bitarray.h>

#include "../lfs-config.h"
#include "filesystem.h"
#include "bitmap.h"
#include "manejo-datos.h"
#include "bitmap.h"
#include "../lfs-logger.h"

int tamanio_bloque = -1;
int cantidad_bloques = -1;

int get_tamanio_bloque() {
	return tamanio_bloque;
}

int get_cantidad_bloques() {
	return cantidad_bloques;
}

int inicializar_filesystem(){
	char path[256];
	char* punto_montaje = get_punto_montaje();
	sprintf(path,"%s/Metadata/Metadata.bin", punto_montaje);
	t_config *metadata = config_create(path);
	if (metadata == NULL){
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "No se pudo abrir el archivo metadata");
		return -1;
	}

	tamanio_bloque = config_get_int_value(metadata, "BLOCK_SIZE");
	cantidad_bloques = config_get_int_value(metadata, "BLOCKS");
	config_destroy(metadata);

	int _crear_directorio_en_punto_montaje(char *nombre) {
		sprintf(path, "%s/%s", punto_montaje, nombre);
		if(mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR 
			| S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
			return errno != EEXIST ? -1 : 0;
		}
		return 0;
	}

	if (_crear_directorio_en_punto_montaje("Tables") < 0) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear el directorio Tables en el punto de montaje");
		return -1;
	}

	if (_crear_directorio_en_punto_montaje("Bloques") < 0) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear el directorio Bloques en el punto de montaje");
		return -1;
	}

	if(crear_bitmap(cantidad_bloques) < 0){
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear el bitmap");
		return -1;
	}

	return 0;
}

void obtener_path_tabla(char *nombre_tabla, char *path) {
	char temporal[TAMANIO_PATH];

	strcpy(temporal, get_punto_montaje());
	strcat(temporal, NOMBRE_DIRECTORIO_TABLAS);
	strcat(temporal, nombre_tabla);
	strcat(temporal, "/");
	strcpy(path, temporal);
}

void obtener_path_particion(int numero, char* nombre_tabla, char* path) {
	char temporal[TAMANIO_PATH];
	char path_tabla[TAMANIO_PATH];
	char numero_string[10];

	obtener_path_tabla(nombre_tabla, path_tabla);
	strcpy(temporal, path_tabla);
	strcat(temporal, nombre_tabla);
	strcat(temporal, "-");
	campo_entero_a_string(numero, numero_string);
	strcat(temporal, numero_string);
	strcat(temporal, ".bin");
	strcpy(path, temporal);
}

void obtener_path_temporal(int numero, char* nombre_tabla, char* path){
	char temporal[TAMANIO_PATH];
	char path_tabla[TAMANIO_PATH];
	char numero_string[10];

	obtener_path_tabla(nombre_tabla, path_tabla);
	strcpy(temporal, path_tabla);
	strcat(temporal, nombre_tabla);
	strcat(temporal, "-T");
	campo_entero_a_string(numero, numero_string);
	strcat(temporal, numero_string);
	strcat(temporal, ".tmp");
	strcpy(path, temporal);
}

void campo_entero_a_string(int entero, char* string) {
	sprintf(string, "%d", entero);
}





