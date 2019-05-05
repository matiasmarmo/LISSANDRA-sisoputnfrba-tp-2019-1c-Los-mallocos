#ifndef LISSANDRA_SRC_MEMORIA_MEMORIA_LOGGER_H_
#define LISSANDRA_SRC_MEMORIA_MEMORIA_LOGGER_H_

int inicializar_memoria_logger();
void destruir_memoria_logger();

void memoria_log_to_level(t_log_level, bool, char*, ...);

#endif
