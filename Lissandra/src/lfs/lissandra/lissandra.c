#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <commons/string.h>

#include "../../commons/comunicacion/protocol.h"
#include "../lfs-config.h"
#include "../lfs-logger.h"
#include "../filesystem/filesystem.h"
#include "../filesystem/manejo-tablas.h"
#include "../filesystem/manejo-datos.h"
#include "memtable.h"

int manejar_create(void* create_request, void* create_response) {
	struct create_request* create_rq = (struct create_request*) create_request;
	struct create_response* create_rp =
			(struct create_response*) create_response;

	memset(create_response, 0, CREATE_RESPONSE_SIZE);
	string_to_upper(create_rq->tabla);

	if (existe_tabla(create_rq->tabla) == 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla %s ya existe",
				create_rq->tabla);
		if (init_error_msg(0, "La tabla ya existe", create_response) < 0) {
			return -1;
		}
		return 0;
	}

	if (init_create_response(0, create_rq->tabla, create_rq->consistencia,
			create_rq->n_particiones, create_rq->t_compactaciones, create_rp)
			< 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al inicializar el create_response");
		return -1;
	}

	metadata_t metadata;
	metadata.consistencia = create_rq->consistencia;
	metadata.n_particiones = create_rq->n_particiones;
	metadata.t_compactaciones = create_rq->t_compactaciones;
	if (crear_tabla(create_rq->tabla, metadata) == -1) {
		destroy(create_rq);
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "Fallo al crear la tabla");
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

	if (existe_tabla(s_describe_rq->tabla) != 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla no existe");
		char buf[100] = { 0 };
		sprintf(buf, "La tabla %s no existe", s_describe_rq->tabla);
		if (init_error_msg(0, buf, single_describe_response) < 0) {
			return -1;
		}
		return 0;
	}

	metadata_t metadata_tabla;
	if (obtener_metadata_tabla(s_describe_rq->tabla, &metadata_tabla) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al obtener la metadata");
		return -1;
	}

	if (init_single_describe_response(0, s_describe_rq->tabla,
			metadata_tabla.consistencia, metadata_tabla.n_particiones,
			metadata_tabla.t_compactaciones, s_describe_rp) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallor al inicializar single_describe_response");
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
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo en obtecion de las metadatas de las tablas");
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
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al inicializar global_describe_response");
		return -1;
	}

	void destruir_elemento(void *elemento) {
		free(elemento);
	}

	list_destroy_and_destroy_elements(tablas, &destruir_elemento);
	list_destroy_and_destroy_elements(metadatas, &destruir_elemento);

	return 0;
}

int manejar_describe(void* describe_request, void* describe_response) {
	struct describe_request* s_describe_rq =
			(struct describe_request*) describe_request;
	if (s_describe_rq->todas) {
		return manejar_global_describe(describe_response);
	} else {
		return manejar_single_describe(describe_request, describe_response);
	}
}

int manejar_drop(void* drop_request, void* drop_response) {
	struct drop_request* drop_rq = (struct drop_request*) drop_request;
	struct drop_response* drop_rp = (struct drop_response*) drop_response;

	memset(drop_response, 0, DROP_RESPONSE_SIZE);
	string_to_upper(drop_rq->tabla);

	if (existe_tabla(drop_rq->tabla) != 0) {
		char buf[100] = { 0 };
		sprintf(buf, "La tabla %s no existe", drop_rq->tabla);
		if (init_error_msg(0, buf, drop_response) < 0) {
			return -1;
		}
		return 0;
	}

	if (init_drop_response(0, drop_rq->tabla, drop_rp) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al inicializar drop_response");
		return -1;
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
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla %s no existe",
				insert_rq->tabla);
		char buf[100] = { 0 };
		sprintf(buf, "La tabla %s no existe", insert_rq->tabla);
		return init_error_msg(0, buf, insert_response);
	}

    if(strlen(insert_rq->valor) > get_tamanio_value()) {
        char buf[100] = { 0 };
        sprintf(buf, "El tamaño máximo del value es %d", get_tamanio_value());
        return init_error_msg(0, buf, insert_response);
    }

	registro_t registro;
	registro.key = insert_rq->key;
	registro.value = insert_rq->valor;
	registro.timestamp = insert_rq->timestamp;
	if (insert_rq->timestamp == 0) {
		registro.timestamp = time(NULL);
	}

	if (insertar_en_memtable(registro, insert_rq->tabla) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al insertar en memtable key: %d valor: %s ",
				insert_rq->key, insert_rq->valor);
		return -1;
	}

	if (init_insert_response(0, insert_rq->tabla, insert_rq->key,
			insert_rq->valor, insert_rq->timestamp, insert_rp) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al inicializar insert_response");
		return -1;
	}

	return 0;
}

int manejar_select(void* select_request, void* select_response) {
	struct select_request* select_rq = (struct select_request*) select_request;
	struct select_response* select_rp =
			(struct select_response*) select_response;

	memset(select_response, 0, SELECT_RESPONSE_SIZE);
	string_to_upper(select_rq->tabla);

	if (existe_tabla(select_rq->tabla) != 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla %s no existe",
				select_rq->tabla);
		char buf[100] = { 0 };
		sprintf(buf, "La tabla %s no existe", select_rq->tabla);
		if (init_error_msg(0, buf, select_response) < 0) {
			return -1;
		}
		return 0;
	}

	metadata_t metadata_tabla;
	if (obtener_metadata_tabla(select_rq->tabla, &metadata_tabla) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al obtener la metadata de la tabla %s",
				select_rq->tabla);
		return -1;
	}

	registro_t resultado;
	resultado.value = NULL;
	resultado.timestamp = 0;
	int cantidad_temporales = 0;

	int _manejar_registro(registro_t nuevo_registro) {
		if (nuevo_registro.key == select_rq->key
				&& resultado.timestamp < nuevo_registro.timestamp) {
			free(resultado.value);
			resultado = nuevo_registro;
		} else {
			free(nuevo_registro.value);
		}
		return CONTINUAR;
	}

	if ((cantidad_temporales = cantidad_tmp_en_tabla(select_rq->tabla)) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al obtener la cantidad de temporales en la tabla %s",
				select_rq->tabla);
		return -1;
	}

	int particion = select_rq->key % metadata_tabla.n_particiones;

	if (iterar_particion(select_rq->tabla, particion, &_manejar_registro) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al iterar en particion %d tabla %s", particion,
				select_rq->tabla);
		return -1;
	}

	for (int i = 0; i < cantidad_temporales; i++) {
		iterar_archivo_temporal(select_rq->tabla, i, &_manejar_registro);
	}

	if (iterar_entrada_memtable(select_rq->tabla, &_manejar_registro) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo iterar entrada de la tabla %s en memtable",
				select_rq->tabla);
		return -1;
	}

	if (resultado.value == NULL) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "No existe el registro %d",
				select_rq->key);
		char buf[100] = { 0 };
		sprintf(buf, "No existe el registro %d", select_rq->key);
		if (init_error_msg(0, buf, select_response) < 0) {
			return -1;
		}
		return 0;
	}

	if (init_select_response(0, select_rq->tabla, select_rq->key,
			resultado.value, resultado.timestamp, select_rp) < 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false,
				"Fallo al inicializar el select_response");
		return -1;
	}

	free(resultado.value);
	return 0;
}

int manejar_request(uint8_t* buffer, uint8_t* respuesta) {
	int id_request = get_msg_id(buffer);
	switch (id_request) {
	case SELECT_REQUEST_ID:
		return manejar_select(buffer, respuesta);
	case INSERT_REQUEST_ID:
		return manejar_insert(buffer, respuesta);
	case CREATE_REQUEST_ID:
		return manejar_create(buffer, respuesta);
	case DESCRIBE_REQUEST_ID:
		return manejar_describe(buffer, respuesta);
	case DROP_REQUEST_ID:
		return manejar_drop(buffer, respuesta);
	}
	return 0;
}
