#ifndef MEMORIA_MEMORIA_REQUEST_HANDLER_H_
#define MEMORIA_MEMORIA_REQUEST_HANDLER_H_

int _manejar_select(struct select_request, void* respuesta_select);
int _manejar_insert(struct insert_request, void* respuesta_insert);
int _manejar_create(struct create_request, void* respuesta_create);

#endif /* MEMORIA_MEMORIA_REQUEST_HANDLER_H_ */
