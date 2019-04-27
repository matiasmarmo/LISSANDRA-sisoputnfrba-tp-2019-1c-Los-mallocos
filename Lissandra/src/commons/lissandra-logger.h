#ifndef LISSANDRA_LOGGER_H_
#define LISSANDRA_LOGGER_H_

#include <commons/log.h>

t_log *lissandra_log_create(char*, char*);

void lissandra_log_to_level(t_log*, t_log_level, char*);

#endif
