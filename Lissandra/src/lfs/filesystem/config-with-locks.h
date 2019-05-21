#ifndef CONFIG_WITH_LOCKS_H_
#define CONFIG_WITH_LOCKS_H_

#include <commons/config.h>

t_config *lfs_config_create_from_file(char *path, FILE *file);

int lfs_config_save_in_file(t_config *self, FILE *file);

#endif