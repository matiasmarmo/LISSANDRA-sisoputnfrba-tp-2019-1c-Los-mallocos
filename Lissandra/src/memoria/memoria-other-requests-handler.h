#ifndef MEMORIA_MEMORIA_OTHER_REQUESTS_HANDLER_H_
#define MEMORIA_MEMORIA_OTHER_REQUESTS_HANDLER_H_

int _manejar_create(struct create_request, void* respuesta_create);
int _manejar_describe(struct describe_request, void* respuesta_describe);
int _manejar_drop(struct drop_request, void* respuesta_drop);
int _manejar_gossip(struct gossip, void *);

#endif /* MEMORIA_MEMORIA_OTHER_REQUESTS_HANDLER_H_ */
