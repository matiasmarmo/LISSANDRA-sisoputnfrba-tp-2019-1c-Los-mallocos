#ifndef COMPACTADOR_H_
#define COMPACTADOR_H_

#include "filesystem/manejo-tablas.h"

int inicializar_compactador();

int destruir_compactador();

void *compactar(void *);

int instanciar_hilo_compactador(char *tabla, int t_compactaciones);

int finalizar_hilo_compactador(char *tabla);

#endif