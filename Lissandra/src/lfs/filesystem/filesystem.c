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
#include <ftw.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/string.h>

#include "../lfs-config.h"
#include "../lfs-logger.h"
#include "../compactador.h"
#include "filesystem.h"
#include "bitmap.h"
#include "manejo-datos.h"
#include "manejo-tablas.h"
#include "bitmap.h"

int tamanio_bloque = -1;
int cantidad_bloques = -1;

int get_tamanio_bloque() {
	return tamanio_bloque;
}

int get_cantidad_bloques() {
	return cantidad_bloques;
}

int iterar_tablas(int (funcion)(const char*, const struct stat*, int)) {
	char path[TAMANIO_PATH] = { 0 };
	sprintf(path, "%sTables/", get_punto_montaje());
	return ftw(path, funcion, 10);
}

int crear_compactador_tabla(char *tabla) {
	metadata_t metadata;
	if(obtener_metadata_tabla(tabla, &metadata) < 0) {
		return -1;
	}
	if(instanciar_hilo_compactador(tabla, metadata.t_compactaciones) < 0) {
		return -1;
	}
	if(crear_semaforo_tabla(tabla) < 0) {
		finalizar_hilo_compactador(tabla);
		return -1;
	}
	return 0;
}

int crear_compactadores_de_tablas_existentes() {
	int _crear(const char* path, const struct stat* stat, int flag) {
		if(!string_ends_with((char*)path, ".bin") && !string_ends_with((char*)path, ".tmp")
				&& !string_ends_with((char*)path, ".tmpc") && !string_ends_with((char*)path, "Tables")) {
			char **partes_path = string_split((char*) path, "/");
			if(partes_path == NULL) {
				return 1;
			}
			char anterior[80];
			for(char **i = partes_path; *i != NULL; i++) {
				strcpy(anterior, *i);
				free(*i);
			}
			free(partes_path);
			return crear_compactador_tabla(anterior);
		}
		return 0;
	}
	return iterar_tablas(&_crear);
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

	if(crear_compactadores_de_tablas_existentes() < 0) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al crear compactadores de tablas existentes");
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





