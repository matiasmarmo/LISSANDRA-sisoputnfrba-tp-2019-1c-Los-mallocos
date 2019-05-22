#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <ftw.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#include "../../commons/comunicacion/protocol.h"
#include "../lfs-config.h"
#include "manejo-tablas.h"
#include "filesystem.h"
#include "bitmap.h"
#include "manejo-datos.h"

int crear_directorio_tabla(char* nombre_tabla) {
	int res_mkdir;
	char path_tabla[TAMANIO_PATH];

	obtener_path_tabla(nombre_tabla, path_tabla);
	if ((res_mkdir = mkdir(path_tabla,
	S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH))
			== -1) {
		return -1;
	}
	return 0;
}

int crear_metadata_tabla(metadata_t metadata, char* nombre_tabla) {
	char escritura[200];
	char consistencia_string[10];
	char n_particiones_string[15];
	char t_compactaciones_string[15];
	char nombre_metadata[TAMANIO_PATH];
	char path_tabla[TAMANIO_PATH];
	FILE* metadata_tabla_f;

	obtener_path_tabla(nombre_tabla, path_tabla);
	strcpy(nombre_metadata, path_tabla);
	strcat(nombre_metadata, "/");
	strcat(nombre_metadata, nombre_tabla);
	strcat(nombre_metadata, "-metadata");
	strcat(nombre_metadata, ".bin");
	if ((metadata_tabla_f = abrir_archivo_para_escritura(nombre_metadata))
			== NULL) {
		return -1;
	}

	strcpy(escritura, "CONSISTENCY=");
	campo_entero_a_string(metadata.consistencia, consistencia_string);
	strcat(escritura, consistencia_string);
	strcat(escritura, "\nPARTITIONS=");
	campo_entero_a_string(metadata.n_particiones, n_particiones_string);
	strcat(escritura, n_particiones_string);
	strcat(escritura, "\nCOMPACTION_TIME=");
	campo_entero_a_string(metadata.t_compactaciones, t_compactaciones_string);
	strcat(escritura, t_compactaciones_string);

	if (fwrite(escritura, strlen(escritura), 1, metadata_tabla_f) < 0) {
		fclose(metadata_tabla_f);
		return -1;
	}

	fclose(metadata_tabla_f);
	return 0;
}

int crear_particion(int numero, char* nombre_tabla) {
	FILE* binario_f;
	char nombre_binario[TAMANIO_PATH] = { 0 }, escritura[200] = { 0 };
	char block_string[10];
	int bloque;

	bloque = pedir_bloque_bitmap();
	if (bloque < 0) {
		return -1;
	}
	obtener_path_particion(numero, nombre_tabla, nombre_binario);

	if ((binario_f = abrir_archivo_para_escritura(nombre_binario)) == NULL) {
		return -1;
	}

	strcpy(escritura, "SIZE=0");
	strcat(escritura, "\nBLOCKS=[");
	campo_entero_a_string(bloque, block_string);
	strcat(escritura, block_string);
	strcat(escritura, "]");

	if (fwrite(escritura, strlen(escritura), 1, binario_f) < 0) {
		fclose(binario_f);
		return -1;
	}
	fclose(binario_f);

	return 0;
}

int crear_particiones(int n_particiones, char* nombre_tabla) {

	for (int numero = 0; numero < n_particiones; numero++) {
		if (crear_particion(numero, nombre_tabla) < 0) {
			return -1;
		}
	}

	return 0;
}

int crear_tabla(char* nombre_tabla, metadata_t metadata) {
	int res_tabla;

	if ((res_tabla = crear_directorio_tabla(nombre_tabla)) == -1) {
		return -1;
	}
	if ((res_tabla = crear_metadata_tabla(metadata, nombre_tabla)) == -1) {
		borrar_tabla(nombre_tabla);
		return -1;
	}
	if ((res_tabla = crear_particiones(metadata.n_particiones, nombre_tabla))
			== -1) {
		borrar_tabla(nombre_tabla);
		return -1;
	}

	return 0;
}

int existe_tabla(char* nombre_tabla) {
	char path_tabla[TAMANIO_PATH];
	struct stat buffer;

	obtener_path_tabla(nombre_tabla, path_tabla);

	if (stat(path_tabla, &buffer) < 0) {
		if (errno == ENOENT) {
			return 1;
		}
		return -1;
	}
	return 0;
}

int obtener_metadata_tabla(char* nombre_tabla, metadata_t* metadata_tabla) {
	char buffer[TAMANIO_PATH];

	obtener_path_tabla(nombre_tabla, buffer);
	strcat(buffer, nombre_tabla);
	strcat(buffer, "-metadata.bin");

	t_config* metadata_config_tabla = config_create(buffer);
	if (metadata_config_tabla == NULL) {
		return -1;
	}

	metadata_tabla->consistencia = config_get_int_value(metadata_config_tabla,
			"CONSISTENCY");
	metadata_tabla->n_particiones = config_get_int_value(metadata_config_tabla,
			"PARTITIONS");
	metadata_tabla->t_compactaciones = config_get_int_value(
			metadata_config_tabla, "COMPACTION_TIME");

	config_destroy(metadata_config_tabla);

	return 0;
}

int iterar_directorio_tabla(char *tabla, int (funcion)(const char*, const struct stat*, int)) {

	char path_tabla[TAMANIO_PATH] = { 0 };
    obtener_path_tabla(tabla, path_tabla);
    return ftw(path_tabla, funcion, 10);

}

int borrar_tabla(char *tabla) {

    int _borrar_archivo(const char *path, const struct stat *stat, int flag) {
    	// Falta liberar los bloques que las particiones y archivos temporales
    	// tienen asignados
    	remove(path);
    	return 0;
    }

    char path_tabla[TAMANIO_PATH] = { 0 };
    obtener_path_tabla(tabla, path_tabla);
    iterar_directorio_tabla(tabla, &_borrar_archivo);
    return rmdir(path_tabla);
}

void borrar_todos_los_tmpc(char *tabla) {

	int _borrar_archivo(const char *path, const struct stat *stat, int flag) {
    	if(string_ends_with((char*)path, ".tmpc") || string_ends_with((char*)path, ".tmpc/")) {
    		remove(path);
    	}
    	return 0;
    }

    iterar_directorio_tabla(tabla, &_borrar_archivo);
}

int dar_metadata_tablas(t_list* nombre_tablas, t_list* metadatas) {
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

	metadata_t *metadata_temp;
	char *nombre_temp;
	while ((directorio_datos = readdir(directorio)) != NULL && !string_starts_with(directorio_datos->d_name, ".")) {
		if((nombre_temp = strdup(directorio_datos->d_name)) == NULL) {
			hubo_error = 1;
			break;
		}
		if((metadata_temp = malloc(sizeof(metadata_t))) == NULL) {
			free(nombre_temp);
			hubo_error = 1;
			break;
		}
		if(obtener_metadata_tabla(directorio_datos->d_name, metadata_temp) < 0) {
			free(nombre_temp);
			free(metadata_temp);
			hubo_error = 1;
			break;
		}
		list_add(nombre_tablas, nombre_temp);
		list_add(metadatas, metadata_temp);
	}

	closedir(directorio);

	void _destruir(void *elemento) {
		free(elemento);
	}

	if(hubo_error) {
		list_clean_and_destroy_elements(nombre_tablas, &_destruir);
		list_clean_and_destroy_elements(metadatas, &_destruir);
		return -1;
	}

	return 0;
}
