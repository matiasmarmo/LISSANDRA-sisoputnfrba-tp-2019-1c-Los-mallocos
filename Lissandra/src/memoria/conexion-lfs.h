#ifndef CONEXION_LFS_H_
#define CONEXION_LFS_H_

#include <stdbool.h>

int conectar_lfs();

int desconectar_lfs();

int enviar_mensaje_lfs(void *, void *, bool);

#endif