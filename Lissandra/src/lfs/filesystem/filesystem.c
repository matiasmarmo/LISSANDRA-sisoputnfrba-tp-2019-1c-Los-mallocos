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
#include <dirent.h>
#include <commons/config.h>
#include <commons/bitarray.h>
#include <commons/string.h>

#include "../lfs-config.h"
#include "../lfs-logger.h"
#include "manejo-tablas.h"
#include "manejo-datos.h"
#include "../compactador.h"
#include "filesystem.h"
#include "bitmap.h"

int tamanio_bloque = -1;
int cantidad_bloques = -1;

int get_tamanio_bloque() {
	return tamanio_bloque;
}

int get_cantidad_bloques() {
	return cantidad_bloques;
}

int inicializar_tablas_ya_existentes() {
	// Cuando se inicializar el filesystem, puede que ya
	// existan tablas creadas. Esta funcion, para cada una
	// de esas tablas, crea el semáforo para bloquearla
	// y el hilo que la compacta.
	int hubo_error = 0;
	DIR *directorio;
	struct dirent *directorio_datos;

	char directorio_path[TAMANIO_PATH];
	char* punto_montaje = get_punto_montaje();
	sprintf(directorio_path, "%sTables/", punto_montaje);

	directorio = opendir(directorio_path);

	if (directorio == NULL) {
		return -1;
	}

	metadata_t metadata;
	while ((directorio_datos = readdir(directorio))) {
		if (string_starts_with(directorio_datos->d_name, ".")) {
			continue;
		}
		if(obtener_metadata_tabla(directorio_datos->d_name, &metadata) < 0) {
			hubo_error = 1;
			break;
		}
		if(crear_semaforo_tabla(directorio_datos->d_name) < 0) {
			hubo_error = 1;
			break;
		}
		if(instanciar_hilo_compactador(directorio_datos->d_name, metadata.t_compactaciones) < 0) {
			// No hace falta destruir el semaforo porque el lfs
			// va a finalizar si falla esta función
			hubo_error = 1;
			break;
		}
	}

	closedir(directorio);

	return hubo_error ? -1 : 0;
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

	if(inicializar_tablas_ya_existentes() < 0) {
		lfs_log_to_level(LOG_LEVEL_ERROR, false, "Error al inicializar estructuras de tablas ya existentes");
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





