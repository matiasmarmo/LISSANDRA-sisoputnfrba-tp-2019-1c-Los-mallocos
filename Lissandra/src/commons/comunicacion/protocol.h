#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED
#include <stdint.h> 

enum errors { UNKNOWN_ID = -20, BAD_DATA,
	ALLOC_ERROR, BUFFER_TOO_SMALL, PTR_FIELD_TOO_LONG,
	MESSAGE_TOO_BIG, CONN_CLOSED, SOCKET_ERROR = -1 };
enum consistencias { SC, SHC, EC };

#define SELECT_REQUEST_ID 0
#define SELECT_REQUEST_SIZE sizeof(struct select_request)

struct select_request {
	uint8_t id;
	char* tabla;
	uint16_t key;
};

int decode_select_request(void*, void*, int);
int encode_select_request(void*, uint8_t*, int);
int init_select_request(char* tabla, uint16_t key, struct select_request*);
void destroy_select_request(void*);
int pack_select_request(char* tabla, uint16_t key, uint8_t *, int);
int send_select_request(char* tabla, uint16_t key, int);

#define SELECT_RESPONSE_ID 1
#define SELECT_RESPONSE_SIZE sizeof(struct select_response)

struct select_response {
	uint8_t id;
	uint8_t fallo;
	char* tabla;
	uint16_t key;
	char* valor;
	uint64_t timestamp;
};

int decode_select_response(void*, void*, int);
int encode_select_response(void*, uint8_t*, int);
int init_select_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, struct select_response*);
void destroy_select_response(void*);
int pack_select_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, uint8_t *, int);
int send_select_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, int);

#define INSERT_REQUEST_ID 2
#define INSERT_REQUEST_SIZE sizeof(struct insert_request)

struct insert_request {
	uint8_t id;
	char* tabla;
	uint16_t key;
	char* valor;
	uint64_t timestamp;
};

int decode_insert_request(void*, void*, int);
int encode_insert_request(void*, uint8_t*, int);
int init_insert_request(char* tabla, uint16_t key, char* valor, uint64_t timestamp, struct insert_request*);
void destroy_insert_request(void*);
int pack_insert_request(char* tabla, uint16_t key, char* valor, uint64_t timestamp, uint8_t *, int);
int send_insert_request(char* tabla, uint16_t key, char* valor, uint64_t timestamp, int);

#define INSERT_RESPONSE_ID 3
#define INSERT_RESPONSE_SIZE sizeof(struct insert_response)

struct insert_response {
	uint8_t id;
	uint8_t fallo;
	char* tabla;
	uint16_t key;
	char* valor;
	uint64_t timestamp;
};

int decode_insert_response(void*, void*, int);
int encode_insert_response(void*, uint8_t*, int);
int init_insert_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, struct insert_response*);
void destroy_insert_response(void*);
int pack_insert_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, uint8_t *, int);
int send_insert_response(uint8_t fallo, char* tabla, uint16_t key, char* valor, uint64_t timestamp, int);

#define CREATE_REQUEST_ID 4
#define CREATE_REQUEST_SIZE sizeof(struct create_request)

struct create_request {
	uint8_t id;
	char* tabla;
	uint8_t consistencia;
	uint8_t n_particiones;
	uint32_t t_compactaciones;
};

int decode_create_request(void*, void*, int);
int encode_create_request(void*, uint8_t*, int);
int init_create_request(char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, struct create_request*);
void destroy_create_request(void*);
int pack_create_request(char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, uint8_t *, int);
int send_create_request(char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, int);

#define CREATE_RESPONSE_ID 5
#define CREATE_RESPONSE_SIZE sizeof(struct create_response)

struct create_response {
	uint8_t id;
	uint8_t fallo;
	char* tabla;
	uint8_t consistencia;
	uint8_t n_particiones;
	uint32_t t_compactaciones;
};

int decode_create_response(void*, void*, int);
int encode_create_response(void*, uint8_t*, int);
int init_create_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, struct create_response*);
void destroy_create_response(void*);
int pack_create_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, uint8_t *, int);
int send_create_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactaciones, int);

#define DESCRIBE_REQUEST_ID 6
#define DESCRIBE_REQUEST_SIZE sizeof(struct describe_request)

struct describe_request {
	uint8_t id;
	uint8_t todas;
	char* tabla;
};

int decode_describe_request(void*, void*, int);
int encode_describe_request(void*, uint8_t*, int);
int init_describe_request(uint8_t todas, char* tabla, struct describe_request*);
void destroy_describe_request(void*);
int pack_describe_request(uint8_t todas, char* tabla, uint8_t *, int);
int send_describe_request(uint8_t todas, char* tabla, int);

#define SINGLE_DESCRIBE_RESPONSE_ID 7
#define SINGLE_DESCRIBE_RESPONSE_SIZE sizeof(struct single_describe_response)

struct single_describe_response {
	uint8_t id;
	uint8_t fallo;
	char* tabla;
	uint8_t consistencia;
	uint8_t n_particiones;
	uint32_t t_compactacion;
};

int decode_single_describe_response(void*, void*, int);
int encode_single_describe_response(void*, uint8_t*, int);
int init_single_describe_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactacion, struct single_describe_response*);
void destroy_single_describe_response(void*);
int pack_single_describe_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactacion, uint8_t *, int);
int send_single_describe_response(uint8_t fallo, char* tabla, uint8_t consistencia, uint8_t n_particiones, uint32_t t_compactacion, int);

#define GLOBAL_DESCRIBE_RESPONSE_ID 8
#define GLOBAL_DESCRIBE_RESPONSE_SIZE sizeof(struct global_describe_response)

struct global_describe_response {
	uint8_t id;
	uint8_t fallo;
	char* tablas;
	
	uint8_t consistencias_len; 
	uint8_t* consistencias;
	
	uint8_t numeros_particiones_len; 
	uint8_t* numeros_particiones;
	
	uint8_t tiempos_compactaciones_len; 
	uint32_t* tiempos_compactaciones;
};

int decode_global_describe_response(void*, void*, int);
int encode_global_describe_response(void*, uint8_t*, int);
int init_global_describe_response(uint8_t fallo, char* tablas, uint8_t consistencias_len, uint8_t* consistencias, uint8_t numeros_particiones_len, uint8_t* numeros_particiones, uint8_t tiempos_compactaciones_len, uint32_t* tiempos_compactaciones, struct global_describe_response*);
void destroy_global_describe_response(void*);
int pack_global_describe_response(uint8_t fallo, char* tablas, uint8_t consistencias_len, uint8_t* consistencias, uint8_t numeros_particiones_len, uint8_t* numeros_particiones, uint8_t tiempos_compactaciones_len, uint32_t* tiempos_compactaciones, uint8_t *, int);
int send_global_describe_response(uint8_t fallo, char* tablas, uint8_t consistencias_len, uint8_t* consistencias, uint8_t numeros_particiones_len, uint8_t* numeros_particiones, uint8_t tiempos_compactaciones_len, uint32_t* tiempos_compactaciones, int);

#define DROP_REQUEST_ID 9
#define DROP_REQUEST_SIZE sizeof(struct drop_request)

struct drop_request {
	uint8_t id;
	char* tabla;
};

int decode_drop_request(void*, void*, int);
int encode_drop_request(void*, uint8_t*, int);
int init_drop_request(char* tabla, struct drop_request*);
void destroy_drop_request(void*);
int pack_drop_request(char* tabla, uint8_t *, int);
int send_drop_request(char* tabla, int);

#define DROP_RESPONSE_ID 10
#define DROP_RESPONSE_SIZE sizeof(struct drop_response)

struct drop_response {
	uint8_t id;
	uint8_t fallo;
	char* tabla;
};

int decode_drop_response(void*, void*, int);
int encode_drop_response(void*, uint8_t*, int);
int init_drop_response(uint8_t fallo, char* tabla, struct drop_response*);
void destroy_drop_response(void*);
int pack_drop_response(uint8_t fallo, char* tabla, uint8_t *, int);
int send_drop_response(uint8_t fallo, char* tabla, int);

#define JOURNAL_REQUEST_ID 11
#define JOURNAL_REQUEST_SIZE sizeof(struct journal_request)

struct journal_request {
	uint8_t id;
	
};

int decode_journal_request(void*, void*, int);
int encode_journal_request(void*, uint8_t*, int);
int init_journal_request( struct journal_request*);
void destroy_journal_request(void*);
int pack_journal_request( uint8_t *, int);
int send_journal_request( int);

#define JOURNAL_RESPONSE_ID 12
#define JOURNAL_RESPONSE_SIZE sizeof(struct journal_response)

struct journal_response {
	uint8_t id;
	uint8_t fallo;
};

int decode_journal_response(void*, void*, int);
int encode_journal_response(void*, uint8_t*, int);
int init_journal_response(uint8_t fallo, struct journal_response*);
void destroy_journal_response(void*);
int pack_journal_response(uint8_t fallo, uint8_t *, int);
int send_journal_response(uint8_t fallo, int);

#define ADD_REQUEST_ID 13
#define ADD_REQUEST_SIZE sizeof(struct add_request)

struct add_request {
	uint8_t id;
	uint8_t n_memoria;
	uint8_t criterio;
};

int decode_add_request(void*, void*, int);
int encode_add_request(void*, uint8_t*, int);
int init_add_request(uint8_t n_memoria, uint8_t criterio, struct add_request*);
void destroy_add_request(void*);
int pack_add_request(uint8_t n_memoria, uint8_t criterio, uint8_t *, int);
int send_add_request(uint8_t n_memoria, uint8_t criterio, int);

#define ADD_RESPONSE_ID 14
#define ADD_RESPONSE_SIZE sizeof(struct add_response)

struct add_response {
	uint8_t id;
	uint8_t fallo;
	uint8_t n_memoria;
	uint8_t criterio;
};

int decode_add_response(void*, void*, int);
int encode_add_response(void*, uint8_t*, int);
int init_add_response(uint8_t fallo, uint8_t n_memoria, uint8_t criterio, struct add_response*);
void destroy_add_response(void*);
int pack_add_response(uint8_t fallo, uint8_t n_memoria, uint8_t criterio, uint8_t *, int);
int send_add_response(uint8_t fallo, uint8_t n_memoria, uint8_t criterio, int);

#define RUN_REQUEST_ID 15
#define RUN_REQUEST_SIZE sizeof(struct run_request)

struct run_request {
	uint8_t id;
	char* path;
};

int decode_run_request(void*, void*, int);
int encode_run_request(void*, uint8_t*, int);
int init_run_request(char* path, struct run_request*);
void destroy_run_request(void*);
int pack_run_request(char* path, uint8_t *, int);
int send_run_request(char* path, int);

#define RUN_RESPONSE_ID 16
#define RUN_RESPONSE_SIZE sizeof(struct run_response)

struct run_response {
	uint8_t id;
	uint8_t fallo;
	char* path;
};

int decode_run_response(void*, void*, int);
int encode_run_response(void*, uint8_t*, int);
int init_run_response(uint8_t fallo, char* path, struct run_response*);
void destroy_run_response(void*);
int pack_run_response(uint8_t fallo, char* path, uint8_t *, int);
int send_run_response(uint8_t fallo, char* path, int);

#define METRICS_REQUEST_ID 17
#define METRICS_REQUEST_SIZE sizeof(struct metrics_request)

struct metrics_request {
	uint8_t id;
	
};

int decode_metrics_request(void*, void*, int);
int encode_metrics_request(void*, uint8_t*, int);
int init_metrics_request( struct metrics_request*);
void destroy_metrics_request(void*);
int pack_metrics_request( uint8_t *, int);
int send_metrics_request( int);

#define METRICS_RESPONSE_ID 18
#define METRICS_RESPONSE_SIZE sizeof(struct metrics_response)

struct metrics_response {
	uint8_t id;
	uint8_t fallo;
	char* resultado;
};

int decode_metrics_response(void*, void*, int);
int encode_metrics_response(void*, uint8_t*, int);
int init_metrics_response(uint8_t fallo, char* resultado, struct metrics_response*);
void destroy_metrics_response(void*);
int pack_metrics_response(uint8_t fallo, char* resultado, uint8_t *, int);
int send_metrics_response(uint8_t fallo, char* resultado, int);

#define EXIT_REQUEST_ID 19
#define EXIT_REQUEST_SIZE sizeof(struct exit_request)

struct exit_request {
	uint8_t id;
	
};

int decode_exit_request(void*, void*, int);
int encode_exit_request(void*, uint8_t*, int);
int init_exit_request( struct exit_request*);
void destroy_exit_request(void*);
int pack_exit_request( uint8_t *, int);
int send_exit_request( int);

#define GOSSIP_ID 20
#define GOSSIP_SIZE sizeof(struct gossip)

struct gossip {
	uint8_t id;
	
};

int decode_gossip(void*, void*, int);
int encode_gossip(void*, uint8_t*, int);
int init_gossip( struct gossip*);
void destroy_gossip(void*);
int pack_gossip( uint8_t *, int);
int send_gossip( int);

#define GOSSIP_RESPONSE_ID 21
#define GOSSIP_RESPONSE_SIZE sizeof(struct gossip_response)

struct gossip_response {
	uint8_t id;
	
	uint8_t ips_memorias_len; 
	uint32_t* ips_memorias;
	
	uint8_t puertos_memorias_len; 
	uint16_t* puertos_memorias;
};

int decode_gossip_response(void*, void*, int);
int encode_gossip_response(void*, uint8_t*, int);
int init_gossip_response(uint8_t ips_memorias_len, uint32_t* ips_memorias, uint8_t puertos_memorias_len, uint16_t* puertos_memorias, struct gossip_response*);
void destroy_gossip_response(void*);
int pack_gossip_response(uint8_t ips_memorias_len, uint32_t* ips_memorias, uint8_t puertos_memorias_len, uint16_t* puertos_memorias, uint8_t *, int);
int send_gossip_response(uint8_t ips_memorias_len, uint32_t* ips_memorias, uint8_t puertos_memorias_len, uint16_t* puertos_memorias, int);

#define MEMORY_FULL_ID 22
#define MEMORY_FULL_SIZE sizeof(struct memory_full)

struct memory_full {
	uint8_t id;
	
};

int decode_memory_full(void*, void*, int);
int encode_memory_full(void*, uint8_t*, int);
int init_memory_full( struct memory_full*);
void destroy_memory_full(void*);
int pack_memory_full( uint8_t *, int);
int send_memory_full( int);

int decode(void*, void*, int);
int destroy(void*);
int bytes_needed_to_pack(void*);
int send_msg(int, void*);

int pack_msg(uint16_t, void*, uint8_t*);

int recv_msg(int, void*, int);

int get_max_msg_size();
uint8_t get_msg_id(void*);

#endif