#ifndef KERNEL_METADATA_TABLAS_H_
#define KERNEL_METADATA_TABLAS_H_

#include "../../commons/comunicacion/protocol.h"

int inicializar_tablas();
int cargar_tablas(struct global_describe_response);
int obtener_metadata_tablas();
int destruir_tablas();

void *actualizar_tablas_threaded(void*);

int obtener_consistencia_tabla(char*);
int cargar_tabla_desde_create_request(struct create_request req);

#endif
