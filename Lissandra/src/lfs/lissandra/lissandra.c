#include <stdlib.h>
#include <string.h>

#include <commons/string.h>

#include "../../commons/comunicacion/protocol.h"
#include "../lfs-logger.h"
#include "../filesystem/filesystem.h"
#include "../filesystem/manejo-tablas.h"

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

int manejar_single_describe(void* single_describe_request, void* single_describe_response) {
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




















>>>>>>> Stashed changes
