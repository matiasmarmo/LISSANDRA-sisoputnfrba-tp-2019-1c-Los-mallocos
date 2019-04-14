#include <stdio.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "protocol-utils.h"

int uint32_a_ipv4_string(uint32_t ip, char *resultado, int tamanio_maximo) {
	if(tamanio_maximo < 16) {
		return -1;
	}
	uint8_t bytes[4];
	bytes[0] = ip & 0xFF;
	bytes[1] = (ip >> 8) & 0xFF;
	bytes[2] = (ip >> 16) & 0xFF;
	bytes[3] = (ip >> 24) & 0xFF;
	sprintf(resultado, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
	return 0;
}

int ipv4_a_uint32(char *ipv4, uint32_t *resultado) {
	int ret = inet_pton(AF_INET, ipv4, resultado);
	if(ret == 1) {
		// inet_pton no tuvo error
		// La función graba en network byte order, por lo que
		// le aplicamos ntohl.
		*resultado = ntohl(*resultado);
	}
	return ret == 1 ? 0 : -1;
}
