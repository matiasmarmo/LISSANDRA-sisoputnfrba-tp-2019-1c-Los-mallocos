#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <ifaddrs.h>
#include <commons/string.h>

#include "sockets.h"

int get_local_addrinfo(const char* port, struct addrinfo* hints,
		struct addrinfo** server_info) {

	// Wrapper para getaddrinfo cuando se busca la addrinfo propia

	int ret;
	if ((ret = getaddrinfo(NULL, port, hints, server_info)) != 0) {
		return -1;
	}
	return 0;
}

int get_socket(const struct addrinfo* addr, int flag) {

	// Recibe un addrinfo y retorna el file descriptor de un socket
	// asociado a ella

	int socket_fd = socket(addr->ai_family, addr->ai_socktype,
			addr->ai_protocol);
	if (socket_fd == -1) {
		return -1;
	}
	if(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
		close(socket_fd);
		return -1;
	}
	if (flag == FLAG_NON_BLOCK && (fcntl(socket_fd, F_SETFL, O_NONBLOCK) < 0)) {
		close(socket_fd);
		return -1;
	}
	return socket_fd;
}

int get_binded_socket(const struct addrinfo* possible_addrinfo, int flag) {

	// Recibe un addr info e intenta crear un socket asociado
	// a ella y después bindearlo

	int socket_fd;

	if ((socket_fd = get_socket(possible_addrinfo, flag)) == -1) {
		return -1;
	}

	if (bind(socket_fd, possible_addrinfo->ai_addr,
			possible_addrinfo->ai_addrlen) == -1) {
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

int get_connected_socket(const struct addrinfo* possible_addrinfo, int flag) {

	// Recibe un addrinfo e intenta crear un socket asociado
	// a ella y después conectarlo

	int socket_fd;

	if ((socket_fd = get_socket(possible_addrinfo, flag)) == -1) {
		return -1;
	}

	if (connect(socket_fd, possible_addrinfo->ai_addr,
			possible_addrinfo->ai_addrlen) == -1 &&
	errno != EINPROGRESS) {

		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

int loop_addrinfo_list(struct addrinfo* linked_list,
		int (*socket_builder)(const struct addrinfo*, int), int flag) {

	// Recibe una lista enlazada resultado de getaddrinfo y un puntero
	// a una función. La función debe tomar un addrinfo y devolver
	// un socket o -1 si hubo un error.
	// Retorna el primer socket que pueda obtenerse sin error
	// a partir de un addrinfo de la lista enlazada. Si
	// no puede obtenerse ningun socket, informa el error y retorna -1.

	struct addrinfo* p;
	int socket_fd;

	for (p = linked_list; p != NULL; p = p->ai_next) {
		if ((socket_fd = socket_builder(p, flag)) != -1) {
			return socket_fd;
		}
	}

	return -1;
}

int create_socket_server(const char* port, int backlog) {

	// Recibe un puerto y un backlog. Crea y devuelve el file descriptor
	// de un socket TCP que escucha en ese puerto con dicho backlog. Retorna
	// -1 en caso de error.

	int socket_fd;
	struct addrinfo hints, *server_info;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (get_local_addrinfo(port, &hints, &server_info) != 0) {
		return -1;
	}

	if ((socket_fd = loop_addrinfo_list(server_info, &get_binded_socket,
			FLAG_NONE)) == -1) {
		freeaddrinfo(server_info);
		return -1;
	}

	freeaddrinfo(server_info);

	if (listen(socket_fd, backlog) == -1) {
		close(socket_fd);
		return -1;
	}

	return socket_fd;
}

int create_socket_client(const char* host, const char* port, int flag) {

	// Recibe un host y un puerto. Retorna un socket TCP conectado
	// a la direccion host:puerto o -1 en caso de error.

	int socket_fd;
	struct addrinfo hints, *server_info;
	int ret;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((ret = getaddrinfo(host, port, &hints, &server_info)) != 0) {
		return -1;
	}

	if ((socket_fd = loop_addrinfo_list(server_info, &get_connected_socket,
			flag)) == -1) {
		freeaddrinfo(server_info);
		return -1;
	}

	freeaddrinfo(server_info);

	return socket_fd;
}

int wait_for_connection(int socket_fd, int timeout_ms) {

	// Espera a que se conecte un socket no bloqueante

	int ret, sock_opt = 0;
	fd_set rfs;
	struct timeval tv;
	struct sockaddr_storage peer_addr;
	socklen_t sock_len = 0;

	tv.tv_sec = timeout_ms / 1000;
	tv.tv_usec = (timeout_ms % 1000) * 1000;
	FD_ZERO(&rfs);
	FD_SET(socket_fd, &rfs);

	if ((ret = select(socket_fd + 1, NULL, &rfs, NULL, &tv)) <= 0) {
		// Hubo un error en select o se llegó al timeout
		close(socket_fd);
		return -1;
	}
	if ((ret = getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &sock_opt, &sock_len))
			< 0 || sock_opt) {
		// Puede haber fallado tanto getsockopt como el connect
		close(socket_fd);
		return -1;
	}
	// Si getpeername retorna cero, el socket está conectado
	return getpeername(socket_fd, (struct sockaddr*) &peer_addr, &sock_len);
}

int socket_set_blocking(int socket_fd) {
	int flags = fcntl(socket_fd, F_GETFL);
	if (flags == -1) {
		return -1;
	}
	flags &= ~O_NONBLOCK;
	return fcntl(socket_fd, F_SETFL, flags) != -1 ? 0 : -1;
}

int get_ip_propia(char *resultado, int len_resultado) {
	struct ifaddrs *id;
	struct sockaddr_in *sa;
	char *addr;

	if(len_resultado < 16) {
		return -1;
	}

	if (getifaddrs(&id) == -1) {
		return -1;
	}

	for (struct ifaddrs *ifa = id; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET) {
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			addr = inet_ntoa(sa->sin_addr);
			if(string_starts_with(addr, "192")) {
				strcpy(resultado, addr);
				freeifaddrs(id);
				return 0;
			}
		}
	}

	freeifaddrs(id);
	return -1;
}
