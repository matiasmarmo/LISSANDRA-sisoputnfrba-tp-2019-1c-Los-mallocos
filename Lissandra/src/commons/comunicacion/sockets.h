#ifndef SOCKETS_H_
#define SOCKETS_H_

enum flags { FLAG_NONE, FLAG_NON_BLOCK };

int create_socket_server(const char*, int);

int create_socket_client(const char*, const char *, int);

int wait_for_connection(int, int);

int socket_set_blocking(int);

#endif
