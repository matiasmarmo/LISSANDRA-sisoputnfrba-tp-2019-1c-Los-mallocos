#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <endian.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "protocol.h"

#define MAX_STRING_SIZE 2048
#define MAX_PTR_COUNT 1024
#define MAX_ENCODED_SIZE 65535

int _send_full_msg(int, uint8_t*, int);
int get_max_msg_size();

int encoded_select_request_size(void* buffer) {
	struct select_request* msg = (struct select_request*) buffer;
	int encoded_size = 1;
	
	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	encoded_size += sizeof(uint16_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_select_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct select_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct select_request msg;
	msg.id = byte_data[current++];
	
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	msg.key = *((uint16_t*) (byte_data + current));
	current += sizeof(uint16_t);
	
	msg.key = ntohs(msg.key);
	memcpy(decoded_data, &msg, sizeof(struct select_request));
	return 0;
}

int encode_select_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct select_request msg = *((struct select_request*) msg_buffer);

	if((encoded_size = encoded_select_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	msg.key = htons(msg.key);

	int current = 0;
	buff[current++] = msg.id;
	
	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;

	*((uint16_t*)(buff + current)) = msg.key;
	current += sizeof(uint16_t);
	
	msg.key = ntohs(msg.key);
	return encoded_size;
}

int init_select_request(char* tabla, uint16_t key, struct select_request* msg) {
	msg->id = SELECT_REQUEST_ID;
	
	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	msg->key = key;
	return 0;
}

void destroy_select_request(void* buffer) {
	struct select_request* msg = (struct select_request*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
}

int pack_select_request(char* tabla, uint16_t key, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct select_request msg;
	int error, encoded_size;
	if((error = init_select_request(tabla, key, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_select_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_select_request(&msg);
		return encoded_size;
	}
	destroy_select_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_select_request(char* tabla, uint16_t key, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct select_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_select_request(tabla, key, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_select_response_size(void* buffer) {
	struct select_response* msg = (struct select_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	encoded_size += sizeof(uint16_t);

	if(msg->valor == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->valor);

	encoded_size += sizeof(uint64_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_select_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct select_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct select_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	msg.key = *((uint16_t*) (byte_data + current));
	current += sizeof(uint16_t);
	int valor_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.valor = malloc(valor_len + 1);
	if(msg.valor == NULL){
		free(msg.tabla);
		return ALLOC_ERROR;
	}
	memcpy(msg.valor, byte_data + current, valor_len);
	msg.valor[valor_len] = '\0';
	current += valor_len;
	msg.timestamp = *((uint64_t*) (byte_data + current));
	current += sizeof(uint64_t);
	
	msg.key = ntohs(msg.key);
	msg.timestamp = be64toh(msg.timestamp);
	memcpy(decoded_data, &msg, sizeof(struct select_response));
	return 0;
}

int encode_select_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct select_response msg = *((struct select_response*) msg_buffer);

	if((encoded_size = encoded_select_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	msg.key = htons(msg.key);
	msg.timestamp = htobe64(msg.timestamp);

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;

	*((uint16_t*)(buff + current)) = msg.key;
	current += sizeof(uint16_t);

	int valor_len = strlen(msg.valor);
	if(valor_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(valor_len);
	current += 2;
	memcpy(buff + current, msg.valor, valor_len);
	current += valor_len;

	*((uint64_t*)(buff + current)) = msg.timestamp;
	current += sizeof(uint64_t);
	
	msg.key = ntohs(msg.key);
	msg.timestamp = be64toh(msg.timestamp);
	return encoded_size;
}

int init_select_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, struct select_response* msg) {
	msg->id = SELECT_RESPONSE_ID;
		msg->fallo = fallo;

	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	msg->key = key;

	if(valor == NULL) {
		free(msg->tabla);
	msg->tabla = NULL;
		return BAD_DATA;
	}
	msg->valor = malloc(strlen(valor) + 1);
	if(msg->valor == NULL) {
		free(msg->tabla);
	msg->tabla = NULL;
		return ALLOC_ERROR; 
	}
	strcpy(msg->valor, valor);

	msg->timestamp = timestamp;
	return 0;
}

void destroy_select_response(void* buffer) {
	struct select_response* msg = (struct select_response*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
	free(msg->valor);
	msg->valor = NULL;
}

int pack_select_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct select_response msg;
	int error, encoded_size;
	if((error = init_select_response(fallo, tabla, key, valor, timestamp, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_select_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_select_response(&msg);
		return encoded_size;
	}
	destroy_select_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_select_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct select_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_select_response(fallo, tabla, key, valor, timestamp, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_insert_request_size(void* buffer) {
	struct insert_request* msg = (struct insert_request*) buffer;
	int encoded_size = 1;
	
	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	encoded_size += sizeof(uint16_t);

	if(msg->valor == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->valor);

	encoded_size += sizeof(uint64_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_insert_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct insert_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct insert_request msg;
	msg.id = byte_data[current++];
	
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	msg.key = *((uint16_t*) (byte_data + current));
	current += sizeof(uint16_t);
	int valor_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.valor = malloc(valor_len + 1);
	if(msg.valor == NULL){
		free(msg.tabla);
		return ALLOC_ERROR;
	}
	memcpy(msg.valor, byte_data + current, valor_len);
	msg.valor[valor_len] = '\0';
	current += valor_len;
	msg.timestamp = *((uint64_t*) (byte_data + current));
	current += sizeof(uint64_t);
	
	msg.key = ntohs(msg.key);
	msg.timestamp = be64toh(msg.timestamp);
	memcpy(decoded_data, &msg, sizeof(struct insert_request));
	return 0;
}

int encode_insert_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct insert_request msg = *((struct insert_request*) msg_buffer);

	if((encoded_size = encoded_insert_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	msg.key = htons(msg.key);
	msg.timestamp = htobe64(msg.timestamp);

	int current = 0;
	buff[current++] = msg.id;
	
	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;

	*((uint16_t*)(buff + current)) = msg.key;
	current += sizeof(uint16_t);

	int valor_len = strlen(msg.valor);
	if(valor_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(valor_len);
	current += 2;
	memcpy(buff + current, msg.valor, valor_len);
	current += valor_len;

	*((uint64_t*)(buff + current)) = msg.timestamp;
	current += sizeof(uint64_t);
	
	msg.key = ntohs(msg.key);
	msg.timestamp = be64toh(msg.timestamp);
	return encoded_size;
}

int init_insert_request(char* tabla, uint16_t key, char* valor, uint64_t timestamp, struct insert_request* msg) {
	msg->id = INSERT_REQUEST_ID;
	
	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	msg->key = key;

	if(valor == NULL) {
		free(msg->tabla);
	msg->tabla = NULL;
		return BAD_DATA;
	}
	msg->valor = malloc(strlen(valor) + 1);
	if(msg->valor == NULL) {
		free(msg->tabla);
	msg->tabla = NULL;
		return ALLOC_ERROR; 
	}
	strcpy(msg->valor, valor);

	msg->timestamp = timestamp;
	return 0;
}

void destroy_insert_request(void* buffer) {
	struct insert_request* msg = (struct insert_request*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
	free(msg->valor);
	msg->valor = NULL;
}

int pack_insert_request(char* tabla, uint16_t key, char* valor, uint64_t timestamp, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct insert_request msg;
	int error, encoded_size;
	if((error = init_insert_request(tabla, key, valor, timestamp, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_insert_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_insert_request(&msg);
		return encoded_size;
	}
	destroy_insert_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_insert_request(char* tabla, uint16_t key, char* valor, uint64_t timestamp, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct insert_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_insert_request(tabla, key, valor, timestamp, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_insert_response_size(void* buffer) {
	struct insert_response* msg = (struct insert_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	encoded_size += sizeof(uint16_t);

	if(msg->valor == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->valor);

	encoded_size += sizeof(uint64_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_insert_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct insert_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct insert_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	msg.key = *((uint16_t*) (byte_data + current));
	current += sizeof(uint16_t);
	int valor_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.valor = malloc(valor_len + 1);
	if(msg.valor == NULL){
		free(msg.tabla);
		return ALLOC_ERROR;
	}
	memcpy(msg.valor, byte_data + current, valor_len);
	msg.valor[valor_len] = '\0';
	current += valor_len;
	msg.timestamp = *((uint64_t*) (byte_data + current));
	current += sizeof(uint64_t);
	
	msg.key = ntohs(msg.key);
	msg.timestamp = be64toh(msg.timestamp);
	memcpy(decoded_data, &msg, sizeof(struct insert_response));
	return 0;
}

int encode_insert_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct insert_response msg = *((struct insert_response*) msg_buffer);

	if((encoded_size = encoded_insert_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	msg.key = htons(msg.key);
	msg.timestamp = htobe64(msg.timestamp);

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;

	*((uint16_t*)(buff + current)) = msg.key;
	current += sizeof(uint16_t);

	int valor_len = strlen(msg.valor);
	if(valor_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(valor_len);
	current += 2;
	memcpy(buff + current, msg.valor, valor_len);
	current += valor_len;

	*((uint64_t*)(buff + current)) = msg.timestamp;
	current += sizeof(uint64_t);
	
	msg.key = ntohs(msg.key);
	msg.timestamp = be64toh(msg.timestamp);
	return encoded_size;
}

int init_insert_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, struct insert_response* msg) {
	msg->id = INSERT_RESPONSE_ID;
		msg->fallo = fallo;

	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	msg->key = key;

	if(valor == NULL) {
		free(msg->tabla);
	msg->tabla = NULL;
		return BAD_DATA;
	}
	msg->valor = malloc(strlen(valor) + 1);
	if(msg->valor == NULL) {
		free(msg->tabla);
	msg->tabla = NULL;
		return ALLOC_ERROR; 
	}
	strcpy(msg->valor, valor);

	msg->timestamp = timestamp;
	return 0;
}

void destroy_insert_response(void* buffer) {
	struct insert_response* msg = (struct insert_response*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
	free(msg->valor);
	msg->valor = NULL;
}

int pack_insert_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct insert_response msg;
	int error, encoded_size;
	if((error = init_insert_response(fallo, tabla, key, valor, timestamp, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_insert_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_insert_response(&msg);
		return encoded_size;
	}
	destroy_insert_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_insert_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct insert_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_insert_response(fallo, tabla, key, valor, timestamp, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_create_request_size(void* buffer) {
	struct create_request* msg = (struct create_request*) buffer;
	int encoded_size = 1;
	
	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint32_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_create_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct create_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct create_request msg;
	msg.id = byte_data[current++];
	
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	msg.consistencia = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.n_particiones = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.t_compactaciones = *((uint32_t*) (byte_data + current));
	current += sizeof(uint32_t);
	
	msg.t_compactaciones = ntohl(msg.t_compactaciones);
	memcpy(decoded_data, &msg, sizeof(struct create_request));
	return 0;
}

int encode_create_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct create_request msg = *((struct create_request*) msg_buffer);

	if((encoded_size = encoded_create_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	msg.t_compactaciones = htonl(msg.t_compactaciones);

	int current = 0;
	buff[current++] = msg.id;
	
	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;

	*((uint8_t*)(buff + current)) = msg.consistencia;
	current += sizeof(uint8_t);

	*((uint8_t*)(buff + current)) = msg.n_particiones;
	current += sizeof(uint8_t);

	*((uint32_t*)(buff + current)) = msg.t_compactaciones;
	current += sizeof(uint32_t);
	
	msg.t_compactaciones = ntohl(msg.t_compactaciones);
	return encoded_size;
}

int init_create_request(char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, struct create_request* msg) {
	msg->id = CREATE_REQUEST_ID;
	
	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	msg->consistencia = consistencia;
	msg->n_particiones = n_particiones;
	msg->t_compactaciones = t_compactaciones;
	return 0;
}

void destroy_create_request(void* buffer) {
	struct create_request* msg = (struct create_request*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
}

int pack_create_request(char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct create_request msg;
	int error, encoded_size;
	if((error = init_create_request(tabla, consistencia, n_particiones, t_compactaciones, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_create_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_create_request(&msg);
		return encoded_size;
	}
	destroy_create_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_create_request(char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct create_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_create_request(tabla, consistencia, n_particiones, t_compactaciones, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_create_response_size(void* buffer) {
	struct create_response* msg = (struct create_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint32_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_create_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct create_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct create_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	msg.consistencia = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.n_particiones = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.t_compactaciones = *((uint32_t*) (byte_data + current));
	current += sizeof(uint32_t);
	
	msg.t_compactaciones = ntohl(msg.t_compactaciones);
	memcpy(decoded_data, &msg, sizeof(struct create_response));
	return 0;
}

int encode_create_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct create_response msg = *((struct create_response*) msg_buffer);

	if((encoded_size = encoded_create_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	msg.t_compactaciones = htonl(msg.t_compactaciones);

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;

	*((uint8_t*)(buff + current)) = msg.consistencia;
	current += sizeof(uint8_t);

	*((uint8_t*)(buff + current)) = msg.n_particiones;
	current += sizeof(uint8_t);

	*((uint32_t*)(buff + current)) = msg.t_compactaciones;
	current += sizeof(uint32_t);
	
	msg.t_compactaciones = ntohl(msg.t_compactaciones);
	return encoded_size;
}

int init_create_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, struct create_response* msg) {
	msg->id = CREATE_RESPONSE_ID;
		msg->fallo = fallo;

	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	msg->consistencia = consistencia;
	msg->n_particiones = n_particiones;
	msg->t_compactaciones = t_compactaciones;
	return 0;
}

void destroy_create_response(void* buffer) {
	struct create_response* msg = (struct create_response*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
}

int pack_create_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct create_response msg;
	int error, encoded_size;
	if((error = init_create_response(fallo, tabla, consistencia, n_particiones, t_compactaciones, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_create_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_create_response(&msg);
		return encoded_size;
	}
	destroy_create_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_create_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct create_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_create_response(fallo, tabla, consistencia, n_particiones, t_compactaciones, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_describe_request_size(void* buffer) {
	struct describe_request* msg = (struct describe_request*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_describe_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct describe_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct describe_request msg;
	msg.id = byte_data[current++];
	
	msg.todas = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	
	memcpy(decoded_data, &msg, sizeof(struct describe_request));
	return 0;
}

int encode_describe_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct describe_request msg = *((struct describe_request*) msg_buffer);

	if((encoded_size = encoded_describe_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.todas;
	current += sizeof(uint8_t);

	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;
	
	return encoded_size;
}

int init_describe_request(uint8_t todas, char* tabla, struct describe_request* msg) {
	msg->id = DESCRIBE_REQUEST_ID;
		msg->todas = todas;

	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	return 0;
}

void destroy_describe_request(void* buffer) {
	struct describe_request* msg = (struct describe_request*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
}

int pack_describe_request(uint8_t todas, char* tabla, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct describe_request msg;
	int error, encoded_size;
	if((error = init_describe_request(todas, tabla, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_describe_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_describe_request(&msg);
		return encoded_size;
	}
	destroy_describe_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_describe_request(uint8_t todas, char* tabla, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct describe_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_describe_request(todas, tabla, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_single_describe_response_size(void* buffer) {
	struct single_describe_response* msg = (struct single_describe_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint32_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_single_describe_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct single_describe_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct single_describe_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	msg.consistencia = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.n_particiones = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.t_compactacion = *((uint32_t*) (byte_data + current));
	current += sizeof(uint32_t);
	
	msg.t_compactacion = ntohl(msg.t_compactacion);
	memcpy(decoded_data, &msg, sizeof(struct single_describe_response));
	return 0;
}

int encode_single_describe_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct single_describe_response msg = *((struct single_describe_response*) msg_buffer);

	if((encoded_size = encoded_single_describe_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	msg.t_compactacion = htonl(msg.t_compactacion);

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;

	*((uint8_t*)(buff + current)) = msg.consistencia;
	current += sizeof(uint8_t);

	*((uint8_t*)(buff + current)) = msg.n_particiones;
	current += sizeof(uint8_t);

	*((uint32_t*)(buff + current)) = msg.t_compactacion;
	current += sizeof(uint32_t);
	
	msg.t_compactacion = ntohl(msg.t_compactacion);
	return encoded_size;
}

int init_single_describe_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactacion, struct single_describe_response* msg) {
	msg->id = SINGLE_DESCRIBE_RESPONSE_ID;
		msg->fallo = fallo;

	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	msg->consistencia = consistencia;
	msg->n_particiones = n_particiones;
	msg->t_compactacion = t_compactacion;
	return 0;
}

void destroy_single_describe_response(void* buffer) {
	struct single_describe_response* msg = (struct single_describe_response*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
}

int pack_single_describe_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactacion, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct single_describe_response msg;
	int error, encoded_size;
	if((error = init_single_describe_response(fallo, tabla, consistencia, n_particiones, t_compactacion, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_single_describe_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_single_describe_response(&msg);
		return encoded_size;
	}
	destroy_single_describe_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_single_describe_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactacion, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct single_describe_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_single_describe_response(fallo, tabla, consistencia, n_particiones, t_compactacion, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_global_describe_response_size(void* buffer) {
	struct global_describe_response* msg = (struct global_describe_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->tablas == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tablas);


	if(msg->consistencias == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += msg->consistencias_len * sizeof(uint8_t);


	if(msg->numeros_particiones == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += msg->numeros_particiones_len * sizeof(uint8_t);


	if(msg->tiempos_compactaciones == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += msg->tiempos_compactaciones_len * sizeof(uint32_t);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_global_describe_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct global_describe_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct global_describe_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int tablas_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tablas = malloc(tablas_len + 1);
	if(msg.tablas == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tablas, byte_data + current, tablas_len);
	msg.tablas[tablas_len] = '\0';
	current += tablas_len;
	msg.consistencias_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.consistencias = malloc(msg.consistencias_len * sizeof(uint8_t));
	if(msg.consistencias == NULL) {
		free(msg.tablas);
		return ALLOC_ERROR;
	}
	memcpy(msg.consistencias, byte_data + current, msg.consistencias_len * sizeof(uint8_t));
	current += msg.consistencias_len * sizeof(uint8_t);

	msg.numeros_particiones_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.numeros_particiones = malloc(msg.numeros_particiones_len * sizeof(uint8_t));
	if(msg.numeros_particiones == NULL) {
		free(msg.tablas);
free(msg.consistencias);
		return ALLOC_ERROR;
	}
	memcpy(msg.numeros_particiones, byte_data + current, msg.numeros_particiones_len * sizeof(uint8_t));
	current += msg.numeros_particiones_len * sizeof(uint8_t);

	msg.tiempos_compactaciones_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tiempos_compactaciones = malloc(msg.tiempos_compactaciones_len * sizeof(uint32_t));
	if(msg.tiempos_compactaciones == NULL) {
		free(msg.tablas);
free(msg.consistencias);
free(msg.numeros_particiones);
		return ALLOC_ERROR;
	}
	memcpy(msg.tiempos_compactaciones, byte_data + current, msg.tiempos_compactaciones_len * sizeof(uint32_t));
	current += msg.tiempos_compactaciones_len * sizeof(uint32_t);

	
	for(int i = 0; i < msg.tiempos_compactaciones_len; i++) {
		msg.tiempos_compactaciones[i] = ntohl(msg.tiempos_compactaciones[i]);
	}
	memcpy(decoded_data, &msg, sizeof(struct global_describe_response));
	return 0;
}

int encode_global_describe_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct global_describe_response msg = *((struct global_describe_response*) msg_buffer);

	if((encoded_size = encoded_global_describe_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	for(int i = 0; i < msg.tiempos_compactaciones_len; i++) {
		msg.tiempos_compactaciones[i] = htonl(msg.tiempos_compactaciones[i]);
	}

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int tablas_len = strlen(msg.tablas);
	if(tablas_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tablas_len);
	current += 2;
	memcpy(buff + current, msg.tablas, tablas_len);
	current += tablas_len;

	if(msg.consistencias_len > MAX_PTR_COUNT) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(msg.consistencias_len);
	current += 2;
	memcpy(buff + current, msg.consistencias, msg.consistencias_len * sizeof(uint8_t));
	current += msg.consistencias_len * sizeof(uint8_t);

	if(msg.numeros_particiones_len > MAX_PTR_COUNT) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(msg.numeros_particiones_len);
	current += 2;
	memcpy(buff + current, msg.numeros_particiones, msg.numeros_particiones_len * sizeof(uint8_t));
	current += msg.numeros_particiones_len * sizeof(uint8_t);

	if(msg.tiempos_compactaciones_len > MAX_PTR_COUNT) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(msg.tiempos_compactaciones_len);
	current += 2;
	memcpy(buff + current, msg.tiempos_compactaciones, msg.tiempos_compactaciones_len * sizeof(uint32_t));
	current += msg.tiempos_compactaciones_len * sizeof(uint32_t);
	
	for(int i = 0; i < msg.tiempos_compactaciones_len; i++) {
		msg.tiempos_compactaciones[i] = ntohl(msg.tiempos_compactaciones[i]);
	}
	return encoded_size;
}

int init_global_describe_response(uint8_t fallo, char* tablas, uint8_t consistencias_len, uint8_t* consistencias, uint8_t numeros_particiones_len, uint8_t* numeros_particiones, uint8_t tiempos_compactaciones_len, uint32_t* tiempos_compactaciones, struct global_describe_response* msg) {
	msg->id = GLOBAL_DESCRIBE_RESPONSE_ID;
		msg->fallo = fallo;

	if(tablas == NULL) {
		
		return BAD_DATA;
	}
	msg->tablas = malloc(strlen(tablas) + 1);
	if(msg->tablas == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tablas, tablas);


	if(consistencias == NULL) {
		free(msg->tablas);
	msg->tablas = NULL;
		return BAD_DATA;
	}
	msg->consistencias_len = consistencias_len;
	msg->consistencias = malloc(consistencias_len * sizeof(uint8_t));
	if(msg->consistencias == NULL) {
		free(msg->tablas);
	msg->tablas = NULL;
		return BAD_DATA;
	}
	memcpy(msg->consistencias, consistencias, consistencias_len * sizeof(uint8_t));

	if(numeros_particiones == NULL) {
		free(msg->tablas);
	msg->tablas = NULL;
free(msg->consistencias);
	msg->consistencias = NULL;
		return BAD_DATA;
	}
	msg->numeros_particiones_len = numeros_particiones_len;
	msg->numeros_particiones = malloc(numeros_particiones_len * sizeof(uint8_t));
	if(msg->numeros_particiones == NULL) {
		free(msg->tablas);
	msg->tablas = NULL;
free(msg->consistencias);
	msg->consistencias = NULL;
		return BAD_DATA;
	}
	memcpy(msg->numeros_particiones, numeros_particiones, numeros_particiones_len * sizeof(uint8_t));

	if(tiempos_compactaciones == NULL) {
		free(msg->tablas);
	msg->tablas = NULL;
free(msg->consistencias);
	msg->consistencias = NULL;
free(msg->numeros_particiones);
	msg->numeros_particiones = NULL;
		return BAD_DATA;
	}
	msg->tiempos_compactaciones_len = tiempos_compactaciones_len;
	msg->tiempos_compactaciones = malloc(tiempos_compactaciones_len * sizeof(uint32_t));
	if(msg->tiempos_compactaciones == NULL) {
		free(msg->tablas);
	msg->tablas = NULL;
free(msg->consistencias);
	msg->consistencias = NULL;
free(msg->numeros_particiones);
	msg->numeros_particiones = NULL;
		return BAD_DATA;
	}
	memcpy(msg->tiempos_compactaciones, tiempos_compactaciones, tiempos_compactaciones_len * sizeof(uint32_t));
	return 0;
}

void destroy_global_describe_response(void* buffer) {
	struct global_describe_response* msg = (struct global_describe_response*) buffer;
	free(msg->tablas);
	msg->tablas = NULL;
	free(msg->consistencias);
	msg->consistencias = NULL;
	free(msg->numeros_particiones);
	msg->numeros_particiones = NULL;
	free(msg->tiempos_compactaciones);
	msg->tiempos_compactaciones = NULL;
}

int pack_global_describe_response(uint8_t fallo, char* tablas, uint8_t consistencias_len, uint8_t* consistencias, uint8_t numeros_particiones_len, uint8_t* numeros_particiones, uint8_t tiempos_compactaciones_len, uint32_t* tiempos_compactaciones, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct global_describe_response msg;
	int error, encoded_size;
	if((error = init_global_describe_response(fallo, tablas, consistencias_len, consistencias, numeros_particiones_len, numeros_particiones, tiempos_compactaciones_len, tiempos_compactaciones, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_global_describe_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_global_describe_response(&msg);
		return encoded_size;
	}
	destroy_global_describe_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_global_describe_response(uint8_t fallo, char* tablas, uint8_t consistencias_len, uint8_t* consistencias, uint8_t numeros_particiones_len, uint8_t* numeros_particiones, uint8_t tiempos_compactaciones_len, uint32_t* tiempos_compactaciones, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct global_describe_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_global_describe_response(fallo, tablas, consistencias_len, consistencias, numeros_particiones_len, numeros_particiones, tiempos_compactaciones_len, tiempos_compactaciones, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_drop_request_size(void* buffer) {
	struct drop_request* msg = (struct drop_request*) buffer;
	int encoded_size = 1;
	
	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_drop_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct drop_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct drop_request msg;
	msg.id = byte_data[current++];
	
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	
	memcpy(decoded_data, &msg, sizeof(struct drop_request));
	return 0;
}

int encode_drop_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct drop_request msg = *((struct drop_request*) msg_buffer);

	if((encoded_size = encoded_drop_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;
	
	return encoded_size;
}

int init_drop_request(char* tabla, struct drop_request* msg) {
	msg->id = DROP_REQUEST_ID;
	
	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	return 0;
}

void destroy_drop_request(void* buffer) {
	struct drop_request* msg = (struct drop_request*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
}

int pack_drop_request(char* tabla, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct drop_request msg;
	int error, encoded_size;
	if((error = init_drop_request(tabla, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_drop_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_drop_request(&msg);
		return encoded_size;
	}
	destroy_drop_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_drop_request(char* tabla, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct drop_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_drop_request(tabla, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_drop_response_size(void* buffer) {
	struct drop_response* msg = (struct drop_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->tabla == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->tabla);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_drop_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct drop_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct drop_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int tabla_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.tabla = malloc(tabla_len + 1);
	if(msg.tabla == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.tabla, byte_data + current, tabla_len);
	msg.tabla[tabla_len] = '\0';
	current += tabla_len;
	
	memcpy(decoded_data, &msg, sizeof(struct drop_response));
	return 0;
}

int encode_drop_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct drop_response msg = *((struct drop_response*) msg_buffer);

	if((encoded_size = encoded_drop_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int tabla_len = strlen(msg.tabla);
	if(tabla_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(tabla_len);
	current += 2;
	memcpy(buff + current, msg.tabla, tabla_len);
	current += tabla_len;
	
	return encoded_size;
}

int init_drop_response(uint8_t fallo, char* tabla, struct drop_response* msg) {
	msg->id = DROP_RESPONSE_ID;
		msg->fallo = fallo;

	if(tabla == NULL) {
		
		return BAD_DATA;
	}
	msg->tabla = malloc(strlen(tabla) + 1);
	if(msg->tabla == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->tabla, tabla);

	return 0;
}

void destroy_drop_response(void* buffer) {
	struct drop_response* msg = (struct drop_response*) buffer;
	free(msg->tabla);
	msg->tabla = NULL;
}

int pack_drop_response(uint8_t fallo, char* tabla, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct drop_response msg;
	int error, encoded_size;
	if((error = init_drop_response(fallo, tabla, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_drop_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_drop_response(&msg);
		return encoded_size;
	}
	destroy_drop_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_drop_response(uint8_t fallo, char* tabla, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct drop_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_drop_response(fallo, tabla, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_journal_request_size(void* buffer) {
	
	int encoded_size = 1;
	
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_journal_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct journal_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct journal_request msg;
	msg.id = byte_data[current++];
	
	
	memcpy(decoded_data, &msg, sizeof(struct journal_request));
	return 0;
}

int encode_journal_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct journal_request msg = *((struct journal_request*) msg_buffer);

	if((encoded_size = encoded_journal_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	
	return encoded_size;
}

int init_journal_request( struct journal_request* msg) {
	msg->id = JOURNAL_REQUEST_ID;
	
	return 0;
}

void destroy_journal_request(void* buffer) {
	
	
}

int pack_journal_request( uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct journal_request msg;
	int error, encoded_size;
	if((error = init_journal_request( &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_journal_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_journal_request(&msg);
		return encoded_size;
	}
	destroy_journal_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_journal_request( int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct journal_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_journal_request( local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_journal_response_size(void* buffer) {
	
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_journal_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct journal_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct journal_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	
	memcpy(decoded_data, &msg, sizeof(struct journal_response));
	return 0;
}

int encode_journal_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct journal_response msg = *((struct journal_response*) msg_buffer);

	if((encoded_size = encoded_journal_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);
	
	return encoded_size;
}

int init_journal_response(uint8_t fallo, struct journal_response* msg) {
	msg->id = JOURNAL_RESPONSE_ID;
		msg->fallo = fallo;
	return 0;
}

void destroy_journal_response(void* buffer) {
	
	
}

int pack_journal_response(uint8_t fallo, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct journal_response msg;
	int error, encoded_size;
	if((error = init_journal_response(fallo, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_journal_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_journal_response(&msg);
		return encoded_size;
	}
	destroy_journal_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_journal_response(uint8_t fallo, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct journal_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_journal_response(fallo, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_add_request_size(void* buffer) {
	
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint8_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_add_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct add_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct add_request msg;
	msg.id = byte_data[current++];
	
	msg.n_memoria = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.criterio = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	
	memcpy(decoded_data, &msg, sizeof(struct add_request));
	return 0;
}

int encode_add_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct add_request msg = *((struct add_request*) msg_buffer);

	if((encoded_size = encoded_add_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.n_memoria;
	current += sizeof(uint8_t);

	*((uint8_t*)(buff + current)) = msg.criterio;
	current += sizeof(uint8_t);
	
	return encoded_size;
}

int init_add_request(uint8_t n_memoria, uint8_t criterio, struct add_request* msg) {
	msg->id = ADD_REQUEST_ID;
		msg->n_memoria = n_memoria;
	msg->criterio = criterio;
	return 0;
}

void destroy_add_request(void* buffer) {
	
	
}

int pack_add_request(uint8_t n_memoria, uint8_t criterio, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct add_request msg;
	int error, encoded_size;
	if((error = init_add_request(n_memoria, criterio, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_add_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_add_request(&msg);
		return encoded_size;
	}
	destroy_add_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_add_request(uint8_t n_memoria, uint8_t criterio, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct add_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_add_request(n_memoria, criterio, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_add_response_size(void* buffer) {
	
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint8_t);
	encoded_size += sizeof(uint8_t);
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_add_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct add_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct add_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.n_memoria = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	msg.criterio = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	
	memcpy(decoded_data, &msg, sizeof(struct add_response));
	return 0;
}

int encode_add_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct add_response msg = *((struct add_response*) msg_buffer);

	if((encoded_size = encoded_add_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	*((uint8_t*)(buff + current)) = msg.n_memoria;
	current += sizeof(uint8_t);

	*((uint8_t*)(buff + current)) = msg.criterio;
	current += sizeof(uint8_t);
	
	return encoded_size;
}

int init_add_response(uint8_t fallo, uint8_t n_memoria, uint8_t criterio, struct add_response* msg) {
	msg->id = ADD_RESPONSE_ID;
		msg->fallo = fallo;
	msg->n_memoria = n_memoria;
	msg->criterio = criterio;
	return 0;
}

void destroy_add_response(void* buffer) {
	
	
}

int pack_add_response(uint8_t fallo, uint8_t n_memoria, uint8_t criterio, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct add_response msg;
	int error, encoded_size;
	if((error = init_add_response(fallo, n_memoria, criterio, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_add_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_add_response(&msg);
		return encoded_size;
	}
	destroy_add_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_add_response(uint8_t fallo, uint8_t n_memoria, uint8_t criterio, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct add_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_add_response(fallo, n_memoria, criterio, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_run_request_size(void* buffer) {
	struct run_request* msg = (struct run_request*) buffer;
	int encoded_size = 1;
	
	if(msg->path == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->path);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_run_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct run_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct run_request msg;
	msg.id = byte_data[current++];
	
	int path_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.path = malloc(path_len + 1);
	if(msg.path == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.path, byte_data + current, path_len);
	msg.path[path_len] = '\0';
	current += path_len;
	
	memcpy(decoded_data, &msg, sizeof(struct run_request));
	return 0;
}

int encode_run_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct run_request msg = *((struct run_request*) msg_buffer);

	if((encoded_size = encoded_run_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	int path_len = strlen(msg.path);
	if(path_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(path_len);
	current += 2;
	memcpy(buff + current, msg.path, path_len);
	current += path_len;
	
	return encoded_size;
}

int init_run_request(char* path, struct run_request* msg) {
	msg->id = RUN_REQUEST_ID;
	
	if(path == NULL) {
		
		return BAD_DATA;
	}
	msg->path = malloc(strlen(path) + 1);
	if(msg->path == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->path, path);

	return 0;
}

void destroy_run_request(void* buffer) {
	struct run_request* msg = (struct run_request*) buffer;
	free(msg->path);
	msg->path = NULL;
}

int pack_run_request(char* path, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct run_request msg;
	int error, encoded_size;
	if((error = init_run_request(path, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_run_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_run_request(&msg);
		return encoded_size;
	}
	destroy_run_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_run_request(char* path, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct run_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_run_request(path, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_run_response_size(void* buffer) {
	struct run_response* msg = (struct run_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->path == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->path);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_run_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct run_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct run_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int path_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.path = malloc(path_len + 1);
	if(msg.path == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.path, byte_data + current, path_len);
	msg.path[path_len] = '\0';
	current += path_len;
	
	memcpy(decoded_data, &msg, sizeof(struct run_response));
	return 0;
}

int encode_run_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct run_response msg = *((struct run_response*) msg_buffer);

	if((encoded_size = encoded_run_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int path_len = strlen(msg.path);
	if(path_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(path_len);
	current += 2;
	memcpy(buff + current, msg.path, path_len);
	current += path_len;
	
	return encoded_size;
}

int init_run_response(uint8_t fallo, char* path, struct run_response* msg) {
	msg->id = RUN_RESPONSE_ID;
		msg->fallo = fallo;

	if(path == NULL) {
		
		return BAD_DATA;
	}
	msg->path = malloc(strlen(path) + 1);
	if(msg->path == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->path, path);

	return 0;
}

void destroy_run_response(void* buffer) {
	struct run_response* msg = (struct run_response*) buffer;
	free(msg->path);
	msg->path = NULL;
}

int pack_run_response(uint8_t fallo, char* path, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct run_response msg;
	int error, encoded_size;
	if((error = init_run_response(fallo, path, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_run_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_run_response(&msg);
		return encoded_size;
	}
	destroy_run_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_run_response(uint8_t fallo, char* path, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct run_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_run_response(fallo, path, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_metrics_request_size(void* buffer) {
	
	int encoded_size = 1;
	
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_metrics_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct metrics_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct metrics_request msg;
	msg.id = byte_data[current++];
	
	
	memcpy(decoded_data, &msg, sizeof(struct metrics_request));
	return 0;
}

int encode_metrics_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct metrics_request msg = *((struct metrics_request*) msg_buffer);

	if((encoded_size = encoded_metrics_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	
	return encoded_size;
}

int init_metrics_request( struct metrics_request* msg) {
	msg->id = METRICS_REQUEST_ID;
	
	return 0;
}

void destroy_metrics_request(void* buffer) {
	
	
}

int pack_metrics_request( uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct metrics_request msg;
	int error, encoded_size;
	if((error = init_metrics_request( &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_metrics_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_metrics_request(&msg);
		return encoded_size;
	}
	destroy_metrics_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_metrics_request( int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct metrics_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_metrics_request( local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_metrics_response_size(void* buffer) {
	struct metrics_response* msg = (struct metrics_response*) buffer;
	int encoded_size = 1;
		encoded_size += sizeof(uint8_t);

	if(msg->resultado == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += strlen(msg->resultado);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_metrics_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct metrics_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct metrics_response msg;
	msg.id = byte_data[current++];
	
	msg.fallo = *((uint8_t*) (byte_data + current));
	current += sizeof(uint8_t);
	int resultado_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.resultado = malloc(resultado_len + 1);
	if(msg.resultado == NULL){
		
		return ALLOC_ERROR;
	}
	memcpy(msg.resultado, byte_data + current, resultado_len);
	msg.resultado[resultado_len] = '\0';
	current += resultado_len;
	
	memcpy(decoded_data, &msg, sizeof(struct metrics_response));
	return 0;
}

int encode_metrics_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct metrics_response msg = *((struct metrics_response*) msg_buffer);

	if((encoded_size = encoded_metrics_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	*((uint8_t*)(buff + current)) = msg.fallo;
	current += sizeof(uint8_t);

	int resultado_len = strlen(msg.resultado);
	if(resultado_len > MAX_STRING_SIZE) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(resultado_len);
	current += 2;
	memcpy(buff + current, msg.resultado, resultado_len);
	current += resultado_len;
	
	return encoded_size;
}

int init_metrics_response(uint8_t fallo, char* resultado, struct metrics_response* msg) {
	msg->id = METRICS_RESPONSE_ID;
		msg->fallo = fallo;

	if(resultado == NULL) {
		
		return BAD_DATA;
	}
	msg->resultado = malloc(strlen(resultado) + 1);
	if(msg->resultado == NULL) {
		
		return ALLOC_ERROR; 
	}
	strcpy(msg->resultado, resultado);

	return 0;
}

void destroy_metrics_response(void* buffer) {
	struct metrics_response* msg = (struct metrics_response*) buffer;
	free(msg->resultado);
	msg->resultado = NULL;
}

int pack_metrics_response(uint8_t fallo, char* resultado, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct metrics_response msg;
	int error, encoded_size;
	if((error = init_metrics_response(fallo, resultado, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_metrics_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_metrics_response(&msg);
		return encoded_size;
	}
	destroy_metrics_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_metrics_response(uint8_t fallo, char* resultado, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct metrics_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_metrics_response(fallo, resultado, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_exit_request_size(void* buffer) {
	
	int encoded_size = 1;
	
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_exit_request (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct exit_request)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct exit_request msg;
	msg.id = byte_data[current++];
	
	
	memcpy(decoded_data, &msg, sizeof(struct exit_request));
	return 0;
}

int encode_exit_request(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct exit_request msg = *((struct exit_request*) msg_buffer);

	if((encoded_size = encoded_exit_request_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	
	return encoded_size;
}

int init_exit_request( struct exit_request* msg) {
	msg->id = EXIT_REQUEST_ID;
	
	return 0;
}

void destroy_exit_request(void* buffer) {
	
	
}

int pack_exit_request( uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct exit_request msg;
	int error, encoded_size;
	if((error = init_exit_request( &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_exit_request(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_exit_request(&msg);
		return encoded_size;
	}
	destroy_exit_request(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_exit_request( int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct exit_request);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_exit_request( local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_gossip_size(void* buffer) {
	
	int encoded_size = 1;
	
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_gossip (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct gossip)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct gossip msg;
	msg.id = byte_data[current++];
	
	
	memcpy(decoded_data, &msg, sizeof(struct gossip));
	return 0;
}

int encode_gossip(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct gossip msg = *((struct gossip*) msg_buffer);

	if((encoded_size = encoded_gossip_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	
	return encoded_size;
}

int init_gossip( struct gossip* msg) {
	msg->id = GOSSIP_ID;
	
	return 0;
}

void destroy_gossip(void* buffer) {
	
	
}

int pack_gossip( uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct gossip msg;
	int error, encoded_size;
	if((error = init_gossip( &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_gossip(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_gossip(&msg);
		return encoded_size;
	}
	destroy_gossip(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_gossip( int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct gossip);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_gossip( local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_gossip_response_size(void* buffer) {
	struct gossip_response* msg = (struct gossip_response*) buffer;
	int encoded_size = 1;
	
	if(msg->ips_memorias == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += msg->ips_memorias_len * sizeof(uint32_t);


	if(msg->puertos_memorias == NULL) {
		return BAD_DATA;
	}
	encoded_size += 2;
	encoded_size += msg->puertos_memorias_len * sizeof(uint16_t);

	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_gossip_response (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct gossip_response)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct gossip_response msg;
	msg.id = byte_data[current++];
	
	msg.ips_memorias_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.ips_memorias = malloc(msg.ips_memorias_len * sizeof(uint32_t));
	if(msg.ips_memorias == NULL) {
		
		return ALLOC_ERROR;
	}
	memcpy(msg.ips_memorias, byte_data + current, msg.ips_memorias_len * sizeof(uint32_t));
	current += msg.ips_memorias_len * sizeof(uint32_t);

	msg.puertos_memorias_len = ntohs(*((uint16_t*)(byte_data + current)));
	current += 2;
	msg.puertos_memorias = malloc(msg.puertos_memorias_len * sizeof(uint16_t));
	if(msg.puertos_memorias == NULL) {
		free(msg.ips_memorias);
		return ALLOC_ERROR;
	}
	memcpy(msg.puertos_memorias, byte_data + current, msg.puertos_memorias_len * sizeof(uint16_t));
	current += msg.puertos_memorias_len * sizeof(uint16_t);

	
	for(int i = 0; i < msg.ips_memorias_len; i++) {
		msg.ips_memorias[i] = ntohl(msg.ips_memorias[i]);
	}
	for(int i = 0; i < msg.puertos_memorias_len; i++) {
		msg.puertos_memorias[i] = ntohs(msg.puertos_memorias[i]);
	}
	memcpy(decoded_data, &msg, sizeof(struct gossip_response));
	return 0;
}

int encode_gossip_response(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct gossip_response msg = *((struct gossip_response*) msg_buffer);

	if((encoded_size = encoded_gossip_response_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	
	for(int i = 0; i < msg.ips_memorias_len; i++) {
		msg.ips_memorias[i] = htonl(msg.ips_memorias[i]);
	}
	for(int i = 0; i < msg.puertos_memorias_len; i++) {
		msg.puertos_memorias[i] = htons(msg.puertos_memorias[i]);
	}

	int current = 0;
	buff[current++] = msg.id;
	
	if(msg.ips_memorias_len > MAX_PTR_COUNT) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(msg.ips_memorias_len);
	current += 2;
	memcpy(buff + current, msg.ips_memorias, msg.ips_memorias_len * sizeof(uint32_t));
	current += msg.ips_memorias_len * sizeof(uint32_t);

	if(msg.puertos_memorias_len > MAX_PTR_COUNT) {
		return PTR_FIELD_TOO_LONG;
	}
	*((uint16_t*)(buff + current)) = htons(msg.puertos_memorias_len);
	current += 2;
	memcpy(buff + current, msg.puertos_memorias, msg.puertos_memorias_len * sizeof(uint16_t));
	current += msg.puertos_memorias_len * sizeof(uint16_t);
	
	for(int i = 0; i < msg.ips_memorias_len; i++) {
		msg.ips_memorias[i] = ntohl(msg.ips_memorias[i]);
	}
	for(int i = 0; i < msg.puertos_memorias_len; i++) {
		msg.puertos_memorias[i] = ntohs(msg.puertos_memorias[i]);
	}
	return encoded_size;
}

int init_gossip_response(uint8_t ips_memorias_len, uint32_t* ips_memorias, uint8_t puertos_memorias_len, uint16_t* puertos_memorias, struct gossip_response* msg) {
	msg->id = GOSSIP_RESPONSE_ID;
	
	if(ips_memorias == NULL) {
		
		return BAD_DATA;
	}
	msg->ips_memorias_len = ips_memorias_len;
	msg->ips_memorias = malloc(ips_memorias_len * sizeof(uint32_t));
	if(msg->ips_memorias == NULL) {
		
		return BAD_DATA;
	}
	memcpy(msg->ips_memorias, ips_memorias, ips_memorias_len * sizeof(uint32_t));

	if(puertos_memorias == NULL) {
		free(msg->ips_memorias);
	msg->ips_memorias = NULL;
		return BAD_DATA;
	}
	msg->puertos_memorias_len = puertos_memorias_len;
	msg->puertos_memorias = malloc(puertos_memorias_len * sizeof(uint16_t));
	if(msg->puertos_memorias == NULL) {
		free(msg->ips_memorias);
	msg->ips_memorias = NULL;
		return BAD_DATA;
	}
	memcpy(msg->puertos_memorias, puertos_memorias, puertos_memorias_len * sizeof(uint16_t));
	return 0;
}

void destroy_gossip_response(void* buffer) {
	struct gossip_response* msg = (struct gossip_response*) buffer;
	free(msg->ips_memorias);
	msg->ips_memorias = NULL;
	free(msg->puertos_memorias);
	msg->puertos_memorias = NULL;
}

int pack_gossip_response(uint8_t ips_memorias_len, uint32_t* ips_memorias, uint8_t puertos_memorias_len, uint16_t* puertos_memorias, uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct gossip_response msg;
	int error, encoded_size;
	if((error = init_gossip_response(ips_memorias_len, ips_memorias, puertos_memorias_len, puertos_memorias, &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_gossip_response(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_gossip_response(&msg);
		return encoded_size;
	}
	destroy_gossip_response(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_gossip_response(uint8_t ips_memorias_len, uint32_t* ips_memorias, uint8_t puertos_memorias_len, uint16_t* puertos_memorias, int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct gossip_response);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_gossip_response(ips_memorias_len, ips_memorias, puertos_memorias_len, puertos_memorias, local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

int encoded_memory_full_size(void* buffer) {
	
	int encoded_size = 1;
	
	if(encoded_size > MAX_ENCODED_SIZE) {
		return MESSAGE_TOO_BIG;
	}
	return encoded_size;
}

int decode_memory_full (void *recv_data, void* decoded_data, int max_decoded_size) {
    
	if(max_decoded_size < sizeof(struct memory_full)) {
		return BUFFER_TOO_SMALL;
	}

	uint8_t* byte_data = (uint8_t*) recv_data;
	int current = 0;
	struct memory_full msg;
	msg.id = byte_data[current++];
	
	
	memcpy(decoded_data, &msg, sizeof(struct memory_full));
	return 0;
}

int encode_memory_full(void* msg_buffer, uint8_t* buff, int max_size) {
	
	int encoded_size = 0;
	struct memory_full msg = *((struct memory_full*) msg_buffer);

	if((encoded_size = encoded_memory_full_size(&msg)) < 0) {
		return encoded_size;
	}
	if(encoded_size > max_size) {
		return BUFFER_TOO_SMALL;
	}

	

	int current = 0;
	buff[current++] = msg.id;
	
	
	return encoded_size;
}

int init_memory_full( struct memory_full* msg) {
	msg->id = MEMORY_FULL_ID;
	
	return 0;
}

void destroy_memory_full(void* buffer) {
	
	
}

int pack_memory_full( uint8_t *buff, int max_size) {
	uint8_t local_buffer[max_size - 2];
	struct memory_full msg;
	int error, encoded_size;
	if((error = init_memory_full( &msg)) < 0) {
		return error;
	}
	if((encoded_size = encode_memory_full(&msg, local_buffer, max_size - 2)) < 0) {
		destroy_memory_full(&msg);
		return encoded_size;
	}
	destroy_memory_full(&msg);
	return pack_msg(encoded_size, local_buffer, buff);
}

int send_memory_full( int socket_fd) {

	int bytes_to_send, ret;
	int current_buffer_size = sizeof(struct memory_full);
	uint8_t* local_buffer = malloc(current_buffer_size);
	if(local_buffer == NULL) {
		return ALLOC_ERROR;
	}

	while((bytes_to_send = pack_memory_full( local_buffer, current_buffer_size)) == BUFFER_TOO_SMALL) {
		current_buffer_size *= 2;
		local_buffer = realloc(local_buffer, current_buffer_size);
		if(local_buffer == NULL) {
			return ALLOC_ERROR;
		}
	}

	if(bytes_to_send < 0) {
		return bytes_to_send;
	}

	ret = _send_full_msg(socket_fd, local_buffer, bytes_to_send);
	free(local_buffer);
	return ret;
}

typedef int (*decoder_t)(void*, void*, int);
typedef void (*destroyer_t)(void*);
typedef int (*encoder_t)(void*, uint8_t*, int);
typedef int (*encoded_size_getter_t)(void*);

int decode(void *data, void *buff, int max_size) {

	uint8_t* byte_data = (uint8_t*) data;

	int msg_id = byte_data[0];
	int body_size;

	// Puntero a la función que decodifica
	decoder_t decoder;

	switch(msg_id) {
	
		case SELECT_REQUEST_ID:
			decoder = &decode_select_request;
			body_size = sizeof(struct select_request);
			break;
	
		case SELECT_RESPONSE_ID:
			decoder = &decode_select_response;
			body_size = sizeof(struct select_response);
			break;
	
		case INSERT_REQUEST_ID:
			decoder = &decode_insert_request;
			body_size = sizeof(struct insert_request);
			break;
	
		case INSERT_RESPONSE_ID:
			decoder = &decode_insert_response;
			body_size = sizeof(struct insert_response);
			break;
	
		case CREATE_REQUEST_ID:
			decoder = &decode_create_request;
			body_size = sizeof(struct create_request);
			break;
	
		case CREATE_RESPONSE_ID:
			decoder = &decode_create_response;
			body_size = sizeof(struct create_response);
			break;
	
		case DESCRIBE_REQUEST_ID:
			decoder = &decode_describe_request;
			body_size = sizeof(struct describe_request);
			break;
	
		case SINGLE_DESCRIBE_RESPONSE_ID:
			decoder = &decode_single_describe_response;
			body_size = sizeof(struct single_describe_response);
			break;
	
		case GLOBAL_DESCRIBE_RESPONSE_ID:
			decoder = &decode_global_describe_response;
			body_size = sizeof(struct global_describe_response);
			break;
	
		case DROP_REQUEST_ID:
			decoder = &decode_drop_request;
			body_size = sizeof(struct drop_request);
			break;
	
		case DROP_RESPONSE_ID:
			decoder = &decode_drop_response;
			body_size = sizeof(struct drop_response);
			break;
	
		case JOURNAL_REQUEST_ID:
			decoder = &decode_journal_request;
			body_size = sizeof(struct journal_request);
			break;
	
		case JOURNAL_RESPONSE_ID:
			decoder = &decode_journal_response;
			body_size = sizeof(struct journal_response);
			break;
	
		case ADD_REQUEST_ID:
			decoder = &decode_add_request;
			body_size = sizeof(struct add_request);
			break;
	
		case ADD_RESPONSE_ID:
			decoder = &decode_add_response;
			body_size = sizeof(struct add_response);
			break;
	
		case RUN_REQUEST_ID:
			decoder = &decode_run_request;
			body_size = sizeof(struct run_request);
			break;
	
		case RUN_RESPONSE_ID:
			decoder = &decode_run_response;
			body_size = sizeof(struct run_response);
			break;
	
		case METRICS_REQUEST_ID:
			decoder = &decode_metrics_request;
			body_size = sizeof(struct metrics_request);
			break;
	
		case METRICS_RESPONSE_ID:
			decoder = &decode_metrics_response;
			body_size = sizeof(struct metrics_response);
			break;
	
		case EXIT_REQUEST_ID:
			decoder = &decode_exit_request;
			body_size = sizeof(struct exit_request);
			break;
	
		case GOSSIP_ID:
			decoder = &decode_gossip;
			body_size = sizeof(struct gossip);
			break;
	
		case GOSSIP_RESPONSE_ID:
			decoder = &decode_gossip_response;
			body_size = sizeof(struct gossip_response);
			break;
	
		case MEMORY_FULL_ID:
			decoder = &decode_memory_full;
			body_size = sizeof(struct memory_full);
			break;
		default:
			return UNKNOWN_ID;
	}

	if(max_size < body_size) {
		return BUFFER_TOO_SMALL;
	}

	decoder(data, buff, body_size);

	return msg_id;
}

int destroy(void* buffer) {
	
	uint8_t* byte_data = (uint8_t*) buffer;
	int msg_id = byte_data[0];
	destroyer_t destroyer;

	switch(msg_id){
	
		case SELECT_REQUEST_ID:
			destroyer = &destroy_select_request;
			break;
	
		case SELECT_RESPONSE_ID:
			destroyer = &destroy_select_response;
			break;
	
		case INSERT_REQUEST_ID:
			destroyer = &destroy_insert_request;
			break;
	
		case INSERT_RESPONSE_ID:
			destroyer = &destroy_insert_response;
			break;
	
		case CREATE_REQUEST_ID:
			destroyer = &destroy_create_request;
			break;
	
		case CREATE_RESPONSE_ID:
			destroyer = &destroy_create_response;
			break;
	
		case DESCRIBE_REQUEST_ID:
			destroyer = &destroy_describe_request;
			break;
	
		case SINGLE_DESCRIBE_RESPONSE_ID:
			destroyer = &destroy_single_describe_response;
			break;
	
		case GLOBAL_DESCRIBE_RESPONSE_ID:
			destroyer = &destroy_global_describe_response;
			break;
	
		case DROP_REQUEST_ID:
			destroyer = &destroy_drop_request;
			break;
	
		case DROP_RESPONSE_ID:
			destroyer = &destroy_drop_response;
			break;
	
		case JOURNAL_REQUEST_ID:
			destroyer = &destroy_journal_request;
			break;
	
		case JOURNAL_RESPONSE_ID:
			destroyer = &destroy_journal_response;
			break;
	
		case ADD_REQUEST_ID:
			destroyer = &destroy_add_request;
			break;
	
		case ADD_RESPONSE_ID:
			destroyer = &destroy_add_response;
			break;
	
		case RUN_REQUEST_ID:
			destroyer = &destroy_run_request;
			break;
	
		case RUN_RESPONSE_ID:
			destroyer = &destroy_run_response;
			break;
	
		case METRICS_REQUEST_ID:
			destroyer = &destroy_metrics_request;
			break;
	
		case METRICS_RESPONSE_ID:
			destroyer = &destroy_metrics_response;
			break;
	
		case EXIT_REQUEST_ID:
			destroyer = &destroy_exit_request;
			break;
	
		case GOSSIP_ID:
			destroyer = &destroy_gossip;
			break;
	
		case GOSSIP_RESPONSE_ID:
			destroyer = &destroy_gossip_response;
			break;
	
		case MEMORY_FULL_ID:
			destroyer = &destroy_memory_full;
			break;
		default:
			return UNKNOWN_ID;
	}

	destroyer(buffer);
	return 0;
}

int bytes_needed_to_pack(void* buffer) {
	
	uint8_t* byte_data = (uint8_t*) buffer;
	int msg_id = byte_data[0];
	encoded_size_getter_t size_getter;

	switch(msg_id){
	
		case SELECT_REQUEST_ID:
			size_getter = &encoded_select_request_size;
			break;
	
		case SELECT_RESPONSE_ID:
			size_getter = &encoded_select_response_size;
			break;
	
		case INSERT_REQUEST_ID:
			size_getter = &encoded_insert_request_size;
			break;
	
		case INSERT_RESPONSE_ID:
			size_getter = &encoded_insert_response_size;
			break;
	
		case CREATE_REQUEST_ID:
			size_getter = &encoded_create_request_size;
			break;
	
		case CREATE_RESPONSE_ID:
			size_getter = &encoded_create_response_size;
			break;
	
		case DESCRIBE_REQUEST_ID:
			size_getter = &encoded_describe_request_size;
			break;
	
		case SINGLE_DESCRIBE_RESPONSE_ID:
			size_getter = &encoded_single_describe_response_size;
			break;
	
		case GLOBAL_DESCRIBE_RESPONSE_ID:
			size_getter = &encoded_global_describe_response_size;
			break;
	
		case DROP_REQUEST_ID:
			size_getter = &encoded_drop_request_size;
			break;
	
		case DROP_RESPONSE_ID:
			size_getter = &encoded_drop_response_size;
			break;
	
		case JOURNAL_REQUEST_ID:
			size_getter = &encoded_journal_request_size;
			break;
	
		case JOURNAL_RESPONSE_ID:
			size_getter = &encoded_journal_response_size;
			break;
	
		case ADD_REQUEST_ID:
			size_getter = &encoded_add_request_size;
			break;
	
		case ADD_RESPONSE_ID:
			size_getter = &encoded_add_response_size;
			break;
	
		case RUN_REQUEST_ID:
			size_getter = &encoded_run_request_size;
			break;
	
		case RUN_RESPONSE_ID:
			size_getter = &encoded_run_response_size;
			break;
	
		case METRICS_REQUEST_ID:
			size_getter = &encoded_metrics_request_size;
			break;
	
		case METRICS_RESPONSE_ID:
			size_getter = &encoded_metrics_response_size;
			break;
	
		case EXIT_REQUEST_ID:
			size_getter = &encoded_exit_request_size;
			break;
	
		case GOSSIP_ID:
			size_getter = &encoded_gossip_size;
			break;
	
		case GOSSIP_RESPONSE_ID:
			size_getter = &encoded_gossip_response_size;
			break;
	
		case MEMORY_FULL_ID:
			size_getter = &encoded_memory_full_size;
			break;
		default:
			return UNKNOWN_ID;
	}

	return size_getter(buffer) + 2;
}

int send_msg(int socket_fd, void* buffer) {
	
	uint8_t* byte_data = (uint8_t*) buffer;
	int msg_id = byte_data[0];
	encoder_t encoder;

	switch(msg_id) {
	
		case SELECT_REQUEST_ID:
			encoder = &encode_select_request;
			break;
	
		case SELECT_RESPONSE_ID:
			encoder = &encode_select_response;
			break;
	
		case INSERT_REQUEST_ID:
			encoder = &encode_insert_request;
			break;
	
		case INSERT_RESPONSE_ID:
			encoder = &encode_insert_response;
			break;
	
		case CREATE_REQUEST_ID:
			encoder = &encode_create_request;
			break;
	
		case CREATE_RESPONSE_ID:
			encoder = &encode_create_response;
			break;
	
		case DESCRIBE_REQUEST_ID:
			encoder = &encode_describe_request;
			break;
	
		case SINGLE_DESCRIBE_RESPONSE_ID:
			encoder = &encode_single_describe_response;
			break;
	
		case GLOBAL_DESCRIBE_RESPONSE_ID:
			encoder = &encode_global_describe_response;
			break;
	
		case DROP_REQUEST_ID:
			encoder = &encode_drop_request;
			break;
	
		case DROP_RESPONSE_ID:
			encoder = &encode_drop_response;
			break;
	
		case JOURNAL_REQUEST_ID:
			encoder = &encode_journal_request;
			break;
	
		case JOURNAL_RESPONSE_ID:
			encoder = &encode_journal_response;
			break;
	
		case ADD_REQUEST_ID:
			encoder = &encode_add_request;
			break;
	
		case ADD_RESPONSE_ID:
			encoder = &encode_add_response;
			break;
	
		case RUN_REQUEST_ID:
			encoder = &encode_run_request;
			break;
	
		case RUN_RESPONSE_ID:
			encoder = &encode_run_response;
			break;
	
		case METRICS_REQUEST_ID:
			encoder = &encode_metrics_request;
			break;
	
		case METRICS_RESPONSE_ID:
			encoder = &encode_metrics_response;
			break;
	
		case EXIT_REQUEST_ID:
			encoder = &encode_exit_request;
			break;
	
		case GOSSIP_ID:
			encoder = &encode_gossip;
			break;
	
		case GOSSIP_RESPONSE_ID:
			encoder = &encode_gossip_response;
			break;
	
		case MEMORY_FULL_ID:
			encoder = &encode_memory_full;
			break;
		default:
			return UNKNOWN_ID;
	}

	int packed_bytes = bytes_needed_to_pack(buffer);
	int encoded_bytes = packed_bytes - 2;
	int error;
	uint8_t encoded[encoded_bytes];
	uint8_t packed[packed_bytes];
	if((error = encoder(buffer, encoded, encoded_bytes)) < 0) {
		return error;
	}
	pack_msg(encoded_bytes, encoded, packed);
	return _send_full_msg(socket_fd, packed, packed_bytes);
}

int struct_size_from_id(uint8_t msg_id) {
	int size = 0;
	switch(msg_id){
	
		case SELECT_REQUEST_ID:
			size = sizeof(struct select_request);
			break;
	
		case SELECT_RESPONSE_ID:
			size = sizeof(struct select_response);
			break;
	
		case INSERT_REQUEST_ID:
			size = sizeof(struct insert_request);
			break;
	
		case INSERT_RESPONSE_ID:
			size = sizeof(struct insert_response);
			break;
	
		case CREATE_REQUEST_ID:
			size = sizeof(struct create_request);
			break;
	
		case CREATE_RESPONSE_ID:
			size = sizeof(struct create_response);
			break;
	
		case DESCRIBE_REQUEST_ID:
			size = sizeof(struct describe_request);
			break;
	
		case SINGLE_DESCRIBE_RESPONSE_ID:
			size = sizeof(struct single_describe_response);
			break;
	
		case GLOBAL_DESCRIBE_RESPONSE_ID:
			size = sizeof(struct global_describe_response);
			break;
	
		case DROP_REQUEST_ID:
			size = sizeof(struct drop_request);
			break;
	
		case DROP_RESPONSE_ID:
			size = sizeof(struct drop_response);
			break;
	
		case JOURNAL_REQUEST_ID:
			size = sizeof(struct journal_request);
			break;
	
		case JOURNAL_RESPONSE_ID:
			size = sizeof(struct journal_response);
			break;
	
		case ADD_REQUEST_ID:
			size = sizeof(struct add_request);
			break;
	
		case ADD_RESPONSE_ID:
			size = sizeof(struct add_response);
			break;
	
		case RUN_REQUEST_ID:
			size = sizeof(struct run_request);
			break;
	
		case RUN_RESPONSE_ID:
			size = sizeof(struct run_response);
			break;
	
		case METRICS_REQUEST_ID:
			size = sizeof(struct metrics_request);
			break;
	
		case METRICS_RESPONSE_ID:
			size = sizeof(struct metrics_response);
			break;
	
		case EXIT_REQUEST_ID:
			size = sizeof(struct exit_request);
			break;
	
		case GOSSIP_ID:
			size = sizeof(struct gossip);
			break;
	
		case GOSSIP_RESPONSE_ID:
			size = sizeof(struct gossip_response);
			break;
	
		case MEMORY_FULL_ID:
			size = sizeof(struct memory_full);
			break;
		default:
			return UNKNOWN_ID;
	}
	return size;
}

int pack_msg(uint16_t body_size, void *msg_body, uint8_t *buff) {
	*((uint16_t*)buff) = htons(body_size);
	memcpy(buff + 2, msg_body, body_size);
	return body_size + 2;
}

int recv_n_bytes(int socket_fd, void* buffer, int bytes_to_read) {
	uint8_t* byte_buffer = (uint8_t*) buffer;
	int bytes_rcvd = 0;
	int num_bytes = 0;
	while(bytes_rcvd < bytes_to_read) {
		num_bytes = recv(socket_fd, byte_buffer + bytes_rcvd, bytes_to_read - bytes_rcvd, 0);
		if(num_bytes == 0) {
			return CONN_CLOSED;
		} else if(num_bytes == -1) {
			return SOCKET_ERROR;
		}
		bytes_rcvd += num_bytes;
	}
	return 0;
}

uint16_t recv_header(int socket_fd) {
	uint16_t header = 0;
	int error = 0;
	if((error = recv_n_bytes(socket_fd, &header, 2)) < 0) {
		return error;
	}
	return ntohs(header);
}

int recv_msg(int socket_fd, void* buffer, int max_size) {

	if(max_size < get_max_msg_size()) {
		return BUFFER_TOO_SMALL;
	}
	
	uint16_t msg_size = 0;
	int error;
	
	if((msg_size = recv_header(socket_fd)) < 0) {
		return msg_size;
	}

	uint8_t local_buffer[msg_size];

	if((error = recv_n_bytes(socket_fd, local_buffer, msg_size)) < 0) {
		return error;
	}

	return decode(local_buffer, buffer, max_size);
}

int _send_full_msg(int socket_fd, uint8_t* buffer, int bytes_to_send) {
	int num_bytes, bytes_sent = 0;
	while(bytes_to_send > bytes_sent) {
		num_bytes = send(socket_fd, buffer + bytes_sent, bytes_to_send - bytes_sent, 0);
		if(num_bytes < 1) {
			return SOCKET_ERROR;
		}
		bytes_sent += num_bytes;
	}
	return bytes_sent;
}

int get_max_msg_size() {
	int sizes[23] = { sizeof(struct select_request),
					sizeof(struct select_response),
					sizeof(struct insert_request),
					sizeof(struct insert_response),
					sizeof(struct create_request),
					sizeof(struct create_response),
					sizeof(struct describe_request),
					sizeof(struct single_describe_response),
					sizeof(struct global_describe_response),
					sizeof(struct drop_request),
					sizeof(struct drop_response),
					sizeof(struct journal_request),
					sizeof(struct journal_response),
					sizeof(struct add_request),
					sizeof(struct add_response),
					sizeof(struct run_request),
					sizeof(struct run_response),
					sizeof(struct metrics_request),
					sizeof(struct metrics_response),
					sizeof(struct exit_request),
					sizeof(struct gossip),
					sizeof(struct gossip_response),
					sizeof(struct memory_full) };
	int max = -1;
	for(int i = 0; i < 23; i++) {
		if(sizes[i] > max) {
			max = sizes[i];
		}
	}
	return max;
}

uint8_t get_msg_id(void* msg) {
	return ((uint8_t*) msg)[0];
}
