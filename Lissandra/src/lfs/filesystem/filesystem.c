#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <time.h>
 #include <string.h>
#include <sys/file.h>
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
	char metadata_path[256];
	char* punto_montaje = get_punto_montaje();
	sprintf(metadata_path,"%s/Metadata/Metadata.bin", punto_montaje);
	t_config *metadata = config_create(metadata_path);
	if (metadata == NULL){
		lfs_log_to_level(LOG_LEVEL_TRACE, false, "No se pudo abrir el archivo metadata");
		return -1;
	}

	tamanio_bloque = config_get_int_value(metadata, "BLOCK_SIZE");
	cantidad_bloques = config_get_int_value(metadata, "BLOCKS");
	config_destroy(metadata);


	if(crear_bitmap(cantidad_bloques) < 0){
		lfs_log_to_level(LOG_LEVEL_TRACE, false, "Error al crear el bitmap");
		return -1;
	}

	return 0;
}

