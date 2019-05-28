#ifndef MANEJO_DATOS_H_
#define MANEJO_DATOS_H_

#include <stdio.h>
#include <commons/config.h>
#include "filesystem.h"

int path_a_bloque(char *numero_bloque, char *buffer, int tamanio_buffer);

void obtener_path_bloque(int nro_bloque, char* path_bloque);

FILE *abrir_archivo_para_lectura(char *path);

FILE *abrir_archivo_para_escritura(char *path);

FILE *abrir_archivo_para_lectoescritura(char *path);

FILE *abrir_archivo_para_agregar(char *path);

int iterar_archivo_de_datos(char *path, operacion_t operacion);

int leer_archivo_de_datos(char *path, registro_t **resultado);

int iterar_particion(char* nombre_tabla, int numero_particion, operacion_t operacion);

int iterar_archivo_temporal(char *nombre_tabla, int numero_temporal, operacion_t operacion);

int escribir_en_archivo_de_datos(char *path, registro_t *registros, int cantidad_registros);

int liberar_bloques_de_archivo(t_config *config_archivo);

int pisar_particion(char* tabla, int nro_particion, registro_t* registros, int cantidad);

int bajar_a_archivo_temporal(char* tabla, registro_t* registros, int cantidad_registros);

int obtener_datos_de_tmpcs(char *tabla, registro_t **resultado);

#endif
