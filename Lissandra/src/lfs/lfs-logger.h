#ifndef LFS_LOGGER_H_
#define LFS_LOGGER_H_

#include <commons/log.h>

int inicializar_lfs_logger();
void destruir_lfs_logger();

void lfs_log_to_level(t_log_level, bool, char*, ...);

#endif
