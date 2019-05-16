#ifndef LECTURA_ARCHIVOS_H_
#define LECTURA_ARCHIVOS_H_

int path_a_bloque(char *numero_bloque, char *buffer, int tamanio_buffer);

int iterar_archivo_de_datos(char *path, operacion_t operacion);

int leer_archivo_de_datos(char *path, registro_t **resultado);

#endif