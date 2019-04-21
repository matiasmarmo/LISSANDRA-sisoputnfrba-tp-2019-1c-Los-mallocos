#ifndef CONSOLA_H_
#define CONSOLA_H_

typedef void (*handler_t)(void*);

void iniciar_consola(handler_t handler);
void leer_siguiente_caracter();
void cerrar_consola();

void mostrar(void*);
void mostrar_async(void*);

#endif
