#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include <commons/bitarray.h>

#include "../lfs-config.h"
#include "filesystem.h"
#include "bitmap.h"
#include "manejo-datos.h"
#include "../lfs-logger.h"

char bitmap_path[TAMANIO_PATH];
int cantidad_bytes_bitmap = -1;
int _cantidad_bloques = -1;

int crear_bitmap(int cantidad_bloques) {
	_cantidad_bloques = cantidad_bloques;
	cantidad_bytes_bitmap = (cantidad_bloques / 8) + 1;
	char datos_iniciales_bitmap[cantidad_bytes_bitmap];
	memset(datos_iniciales_bitmap, 0, cantidad_bytes_bitmap);
	int res_ftell;

	char* punto_montaje = get_punto_montaje();

	sprintf(bitmap_path, "%sMetadata/Bitmap.bin", punto_montaje);
	FILE *archivo_bitmap = abrir_archivo_para_lectoescritura(bitmap_path);
	if (archivo_bitmap == NULL) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false,
				"No se puse abrir el archivo bitmap");
		return -1;
	}

	if(fseek(archivo_bitmap, 0, SEEK_END) < 0) {
		fclose(archivo_bitmap);
		return -1;
	}
	if((res_ftell = ftell(archivo_bitmap)) < 0) {
		fclose(archivo_bitmap);
		return -1;	
	}
	if(res_ftell != 0) {
		// El bitmap ya estaba creado y tiene datos de una 
		// ejecución anterior del lfs. Lo utilizamos a partir
		// de su estado actual
		fclose(archivo_bitmap);
		return 0;
	}

	if (fwrite(datos_iniciales_bitmap, cantidad_bytes_bitmap, 1, archivo_bitmap)
			!= 1) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false, "No se pudo leer");
		fclose(archivo_bitmap);
		return -1;
	}

	fclose(archivo_bitmap);
	return 0;
}

t_bitarray* leer_bitmap(FILE *archivo, char *buffer) {

	if (fread(buffer, cantidad_bytes_bitmap, 1, archivo) != 1) {
		fclose(archivo);
		return NULL;
	}

	return bitarray_create_with_mode(buffer, _cantidad_bloques, LSB_FIRST);
}

int guardar_bitmap_archivo(FILE* archivo, t_bitarray* bitmap) {
	fseek(archivo, 0, SEEK_SET);
	if (fwrite(bitmap->bitarray, cantidad_bytes_bitmap, 1, archivo) != 1) {
		fclose(archivo);
		bitarray_destroy(bitmap);
		return -1;
	}
	fclose(archivo);
	bitarray_destroy(bitmap);
	return 0;
}

int pedir_bloque_bitmap() {
	FILE* bitmap_f = abrir_archivo_para_lectoescritura(bitmap_path);
	if (bitmap_f == NULL) {
		return -1;
	}
	char buffer[cantidad_bytes_bitmap];
	t_bitarray* bitmap = leer_bitmap(bitmap_f, buffer);

	if (bitmap == NULL) {
		fclose(bitmap_f);
		return -1;
	}

	int bloque_elegido = -1;

	for (int i = 0; i < _cantidad_bloques; i++) {
		if (bitarray_test_bit(bitmap, i) == 0) {
			bitarray_set_bit(bitmap, i);
			bloque_elegido = i;
			break;
		}
	}

	if (bloque_elegido == -1) {
		// No hay bloques disponibles
		fclose(bitmap_f);
		bitarray_destroy(bitmap);
		return -1;
	}

	if (guardar_bitmap_archivo(bitmap_f, bitmap) < 0) {
		return -1;
	}
	return bloque_elegido;
}

int liberar_bloque_bitmap(int indice) {
	if(_cantidad_bloques < indice){
		return -1;
	}
	FILE* bitmap_f = abrir_archivo_para_lectoescritura(bitmap_path);
	if (bitmap_f == NULL) {
		return -1;
	}
	char buffer[cantidad_bytes_bitmap];
	t_bitarray* bitmap = leer_bitmap(bitmap_f, buffer);

	if (bitmap == NULL) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false, "No se pudo abrir el bitmap");
		fclose(bitmap_f);
		return -1;
	}

	if (bitarray_test_bit(bitmap, indice) == 0) {
		lfs_log_to_level(LOG_LEVEL_TRACE, false,
				"Error ya esta liberado el bit: %d", indice);
		return -1;
	}

	bitarray_clean_bit(bitmap, indice);

	if (guardar_bitmap_archivo(bitmap_f, bitmap) < 0) {
		return -1;
	}

	return 0;
}
