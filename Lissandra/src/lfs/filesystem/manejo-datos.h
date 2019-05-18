#ifndef MANEJO_DATOS_H_
#define MANEJO_DATOS_H_

#include <stdio.h>

int path_a_bloque(char *numero_bloque, char *buffer, int tamanio_buffer);

FILE *abrir_archivo_para_lectura(char *path);

FILE *abrir_archivo_para_escritura(char *path);

FILE *abrir_archivo_para_agregar(char *path);

int iterar_archivo_de_datos(char *path, operacion_t operacion);

int leer_archivo_de_datos(char *path, registro_t **resultado);

#endif