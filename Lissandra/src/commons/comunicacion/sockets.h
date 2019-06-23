#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdint.h>

enum flags { FLAG_NONE, FLAG_NON_BLOCK };

int create_socket_server(const char*, int);

int create_socket_client(const char*, const char *, int);

int wait_for_connection(int, int);

int socket_set_blocking(int);

int get_ip_propia(char*, int);

#endif
