#ifndef KERNEL_METADATA_TABLAS_H_
#define KERNEL_METADATA_TABLAS_H_

int inicializar_tablas();
int obtener_metadata_tablas();
int destruir_tablas();

void *actualizar_tablas_threaded(void*);

int obtener_consistencia_tabla(char*);

#endif
