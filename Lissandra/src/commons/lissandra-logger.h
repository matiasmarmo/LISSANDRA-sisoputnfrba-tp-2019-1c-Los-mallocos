#ifndef LISSANDRA_LOGGER_H_
#define LISSANDRA_LOGGER_H_

#include <commons/log.h>

extern t_log_level lissandra_log_level;

t_log *lissandra_log_create(char*, char*);

#endif
