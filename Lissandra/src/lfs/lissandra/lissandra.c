#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <commons/string.h>

#include "../../commons/comunicacion/protocol.h"
#include "../lfs-logger.h"
#include "../filesystem/filesystem.h"
#include "../filesystem/manejo-tablas.h"
#include "memtable.h"

// switch ejecutar request en lissandra

int manejar_create(void* create_request, void* create_response) {
	struct create_request* create_rq = (struct create_request*) create_request;
	struct create_response* create_rp =
			(struct create_response*) create_response;

	memset(create_response, 0, CREATE_RESPONSE_SIZE);
	string_to_upper(create_rq->tabla);
	if (init_create_response(0, create_rq->tabla, create_rq->consistencia,
			create_rq->n_particiones, create_rq->t_compactaciones, create_rp)
			< 0) {
		create_rp->fallo = 1;
		return -1;
	}

	if (existe_tabla(create_rq->tabla) == 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla ya existe");
		create_rp->fallo = 1;
		return 0;
	}

	metadata_t metadata;
	metadata.consistencia = create_rq->consistencia;
	metadata.n_particiones = create_rq->n_particiones;
	metadata.t_compactaciones = create_rq->t_compactaciones;
	if (crear_tabla(create_rq->tabla, metadata) == -1) {
		destroy(create_rq);
		return -1;
	}

	return 0;
}

int manejar_single_describe(void* single_describe_request,
		void* single_describe_response) {
	struct describe_request* s_describe_rq =
			(struct describe_request*) single_describe_request;
	struct single_describe_response* s_describe_rp =
			(struct single_describe_response*) single_describe_response;

	memset(single_describe_response, 0, SINGLE_DESCRIBE_RESPONSE_SIZE);
	string_to_upper(s_describe_rq->tabla);
	int res = 0;

	if (existe_tabla(s_describe_rq->tabla) != 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla ya existe");
		res = 1;
	}

	metadata_t metadata_tabla;
	if (obtener_metadata_tabla(s_describe_rq->tabla, &metadata_tabla) < 0) {
		return -1;
	}

	if (init_single_describe_response(res, s_describe_rq->tabla,
			metadata_tabla.consistencia, metadata_tabla.n_particiones,
			metadata_tabla.t_compactaciones, s_describe_rp) < 0) {
		return -1;
	}

	return 0;
}

void lista_a_string_con_centinela(t_list* lista, char* string, char* centinela,
		int tamanio_string) {
	char *elemento;
	memset(string, 0, tamanio_string);

	for (int i = 0; i < list_size(lista); i++) {
		elemento = (char*) list_get(lista, i);
		strcat(string, elemento);

		if ((elemento = (char*) list_get(lista, ++i)) != NULL) {
			strcat(string, ";");
			i--;
		}
	}
}

int manejar_global_describe(void* global_describe_response) {
	struct global_describe_response* g_describe_rp =
			(struct global_describe_response*) global_describe_response;
	int tamanio_nombre_tabla = 50;

	memset(global_describe_response, 0, GLOBAL_DESCRIBE_RESPONSE_SIZE);

	t_list* tablas = list_create();
	t_list* metadatas = list_create();

	if (dar_metadata_tablas(tablas, metadatas) == -1) {
		return -1;
	}

	int tamanio_nombres_tablas = list_size(tablas) * tamanio_nombre_tabla;
	char nombres_tablas[tamanio_nombres_tablas];
	lista_a_string_con_centinela(tablas, nombres_tablas, ";",
			tamanio_nombres_tablas);

	int cantidad_tablas = list_size(tablas);
	uint8_t consistencias[cantidad_tablas];
	uint8_t numeros_particiones[cantidad_tablas];
	uint32_t tiempos_compactaciones[cantidad_tablas];

	obtener_campos_metadatas(consistencias, numeros_particiones,
			tiempos_compactaciones, metadatas);

	if (init_global_describe_response(0, nombres_tablas, cantidad_tablas,
			consistencias, cantidad_tablas, numeros_particiones,
			cantidad_tablas, tiempos_compactaciones, g_describe_rp)) {
		return -1;
	}

	void destruir_elemento(void *elemento) {
		free(elemento);
	}

	list_destroy_and_destroy_elements(tablas, &destruir_elemento);
	list_destroy_and_destroy_elements(metadatas, &destruir_elemento);

	return 0;
}

int manejar_drop(void* drop_request, void* drop_response) {
	struct drop_request* drop_rq = (struct drop_request*) drop_request;
	struct drop_response* drop_rp = (struct drop_response*) drop_response;

	memset(drop_response, 0, DROP_RESPONSE_SIZE);
	string_to_upper(drop_rq->tabla);
	if (init_drop_response(0, drop_rq->tabla, drop_rp) < 0) {
		drop_rp->fallo = 1;
		return -1;
	}

	if (existe_tabla(drop_rq->tabla) != 0) {
		drop_rp->fallo = 1;
		return 0;
	}

	int resultado = borrar_tabla(drop_rq->tabla);
	if (resultado != 0) {
		destroy(drop_rp);
	}
	return resultado;

}

int manejar_insert(void* insert_request, void* insert_response) {
	struct insert_request* insert_rq = (struct insert_request*) insert_request;
	struct insert_response* insert_rp =
			(struct insert_response*) insert_response;

	memset(insert_response, 0, INSERT_RESPONSE_SIZE);
	string_to_upper(insert_rq->tabla);

	if (existe_tabla(insert_rq->tabla) != 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla ya existe");
		return 0;
	}

	metadata_t metadata_tabla;
	if (obtener_metadata_tabla(insert_rq->tabla, &metadata_tabla) < 0) {
		return -1;
	}

	registro_t registro;
	registro.key = insert_rq->key;
	registro.value = insert_rq->valor;
	if (insert_rq->timestamp == 0) {
		registro.timestamp = (unsigned long) time(NULL) / 1000;
	} else {
		registro.timestamp = insert_rq->timestamp;
	}

	if (insertar_en_memtable(registro, insert_rq->tabla) < 0) {
		insert_rp->fallo = 1;
		return -1;
	}
	if (init_insert_response(0, insert_rq->tabla, insert_rq->key,
			insert_rq->valor, insert_rq->timestamp, insert_rp) < 0) {
		return -1;
	}

	return 0;
}
