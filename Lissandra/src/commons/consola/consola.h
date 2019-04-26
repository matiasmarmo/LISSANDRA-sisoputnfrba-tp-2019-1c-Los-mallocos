#ifndef CONSOLA_H_
#define CONSOLA_H_

typedef void (*handler_t)(char*,void*);

void iniciar_consola(handler_t handler);
void leer_siguiente_caracter();
void cerrar_consola();

void mostrar(void*);
void mostrar_async(void*);

void imprimir(char*);
void imprimir_async(char*);

#endif
