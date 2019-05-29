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
