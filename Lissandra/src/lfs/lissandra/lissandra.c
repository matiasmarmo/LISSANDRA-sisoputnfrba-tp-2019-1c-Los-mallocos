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

int manejar_describe_single(void* describe_request, void* describe_response) {
	struct describe_request* describe_rq =
			(struct describe_request*) describe_request;
	struct single_describe_response* describe_rp =
			(struct single_describe_response*) describe_response;

	memset(describe_response, 0, SINGLE_DESCRIBE_RESPONSE_SIZE);
	string_to_upper(describe_rq->tabla);
	int res = 0;

	if (existe_tabla(describe_rq->tabla) != 0) {
		lfs_log_to_level(LOG_LEVEL_WARNING, false, "La tabla ya existe");
		res = 1;
	}

	metadata_t metadata_tabla;
	if (obtener_metadata_tabla(describe_rq->tabla, &metadata_tabla) < 0) {
		return -1;
	}

	if (init_single_describe_response(res, describe_rq->tabla,
			metadata_tabla.consistencia, metadata_tabla.n_particiones,
			metadata_tabla.t_compactaciones, describe_rp) < 0) {
		return -1;
	}

	return 0;
}
