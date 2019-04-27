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

void lissandra_log_to_level(t_log *logger, t_log_level level, char *string) {
	switch (level) {
	case LOG_LEVEL_TRACE:
		log_trace(logger, string);
		break;
	case LOG_LEVEL_DEBUG:
		log_debug(logger, string);
		break;
	case LOG_LEVEL_INFO:
		log_info(logger, string);
		break;
	case LOG_LEVEL_WARNING:
		log_warning(logger, string);
		break;
	case LOG_LEVEL_ERROR:
		log_error(logger, string);
		break;
	}
}
