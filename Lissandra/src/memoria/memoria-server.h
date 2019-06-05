#ifndef LISSANDRA_SRC_MEMORIA_MEMORIA_SERVIDOR_H_
#define LISSANDRA_SRC_MEMORIA_MEMORIA_SERVIDOR_H_

void* correr_servidor_memoria(void*);
void inicializar_clientes();
void destruir_clientes();

typedef struct cliente {
	int cliente_valor;
} cliente_t;

#endif
