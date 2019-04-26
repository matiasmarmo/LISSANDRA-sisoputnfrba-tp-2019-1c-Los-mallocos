#include <stdbool.h>
#include <stdarg.h>
#include <commons/log.h>

#include "lissandra-logger.h"

#ifdef DEBUG
t_log_level lissandra_log_level = LOG_LEVEL_TRACE;
#else
t_log_level lissandra_log_level = LOG_LEVEL_INFO;
#endif

t_log *lissandra_log_create(char *file, char* program) {
	return log_create(file, program, false, lissandra_log_level);
}
