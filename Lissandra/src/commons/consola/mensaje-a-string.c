#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>

#include "mensaje-a-string.h"
#include "../comunicacion/protocol.h"

t_dictionary *diccionario;

typedef int (*convertidor_t)(void*, char*, int);

void obtener_key_string(int key, char* id_string) {
	sprintf(id_string, "%d", key);
}

int convertir_en_string(void* mensaje, char* buffer, int tamanio_maximo) {
	memset(buffer, 0, tamanio_maximo);
	int id = get_msg_id(mensaje);
	char id_string[4];
	obtener_key_string(id, id_string);
	convertidor_t convertidor = dictionary_get(diccionario, id_string);
	return convertidor(mensaje, buffer, tamanio_maximo);
}

int select_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct select_response *response = (struct select_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	char key_string[TAMANIO_MAX_INT_STRING];

	if (response->fallo) {
		strcpy(temporal, "\nFallo SELECT\n");
	} else {
		strcpy(temporal, "\nResultado SELECT\n");
		strcat(temporal, "Tabla: ");
		strcat(temporal, response->tabla);
		strcat(temporal, "\n");
		strcat(temporal, "Key: ");
		obtener_key_string(response->key, key_string);
		strcat(temporal, key_string);
		strcat(temporal, "\n");
		strcat(temporal, "Valor: ");
		strcat(temporal, response->valor);
		strcat(temporal, "\n");
	}

	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int insert_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct insert_response *response = (struct insert_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	char key_string[TAMANIO_MAX_INT_STRING];

	if (response->fallo) {
		strcpy(temporal, "\nFallo INSERT\n");
	} else {
		strcpy(temporal, "\nResultado INSERT\n");
		strcat(temporal, "Tabla: ");
		strcat(temporal, response->tabla);
		strcat(temporal, "\n");
		strcat(temporal, "Key: ");
		obtener_key_string(response->key, key_string);
		strcat(temporal, key_string);
		strcat(temporal, "\n");
		strcat(temporal, "Valor: ");
		strcat(temporal, response->valor);
		strcat(temporal, "\n");
	}

	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int create_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct create_response *response = (struct create_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	char consistencia_string[TAMANIO_MAX_INT_STRING];
	char n_particiones_string[TAMANIO_MAX_INT_STRING];
	char t_compactaciones_string[TAMANIO_MAX_INT_STRING];

	if (response->fallo) {
		strcpy(temporal, "\nFallo CREATE\n");
	} else {
		strcpy(temporal, "\nResultado CREATE\n");
		strcat(temporal, "Tabla: ");
		strcat(temporal, response->tabla);
		strcat(temporal, "\n");
		strcat(temporal, "Consistencia: ");
		obtener_key_string(response->consistencia, consistencia_string);
		strcat(temporal, consistencia_string);
		strcat(temporal, "\n");
		strcat(temporal, "Numero de Particiones: ");
		obtener_key_string(response->n_particiones, n_particiones_string);
		strcat(temporal, n_particiones_string);
		strcat(temporal, "\n");
		strcat(temporal, "Tiempo entre Compactaciones: ");
		obtener_key_string(response->t_compactaciones, t_compactaciones_string);
		strcat(temporal, t_compactaciones_string);
		strcat(temporal, "\n");
	}

	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int single_describe_response_to_string(void* mensaje, char* buffer,
		int tamanio_maximo) {
	struct single_describe_response *response =
			(struct single_describe_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	char consistencia_string[TAMANIO_MAX_INT_STRING];
	char n_particiones_string[TAMANIO_MAX_INT_STRING];
	char t_compactacion_string[TAMANIO_MAX_INT_STRING];

	if (response->fallo) {
		strcpy(temporal, "\nFallo DESCRIBE\n");
	} else {
		strcpy(temporal, "\nResultado DESCRIBE\n");
		strcat(temporal, "Tabla: ");
		strcat(temporal, response->tabla);
		strcat(temporal, "\n");
		strcat(temporal, "Consistencia: ");
		obtener_key_string(response->consistencia, consistencia_string);
		strcat(temporal, consistencia_string);
		strcat(temporal, "\n");
		strcat(temporal, "Numero de Particiones: ");
		obtener_key_string(response->n_particiones, n_particiones_string);
		strcat(temporal, n_particiones_string);
		strcat(temporal, "\n");
		strcat(temporal, "Tiempo entre Compactaciones: ");
		obtener_key_string(response->t_compactacion, t_compactacion_string);
		strcat(temporal, t_compactacion_string);
		strcat(temporal, "\n");
	}

	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int global_describe_response_to_string(void* mensaje, char* buffer,
		int tamanio_maximo) {
	struct global_describe_response *response =
			(struct global_describe_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	char consistencia_string[TAMANIO_MAX_INT_STRING] = { 0 };
	char n_particiones_string[TAMANIO_MAX_INT_STRING] = { 0 };
	char t_compactacion_string[TAMANIO_MAX_INT_STRING] = { 0 };
	char** tablas_string_lista = string_split(response->tablas, ";");

	if (response->fallo) {
		strcpy(temporal, "\nFallo DESCRIBE\n");
	} else {
		strcpy(temporal, "\nResultado DESCRIBE\n");
		for (int i = 0; i < response->consistencias_len; i++) {
			strcat(temporal, "Tabla: ");
			strcat(temporal, tablas_string_lista[i]);
			strcat(temporal, "\n");
			strcat(temporal, "Consistencia: ");
			obtener_key_string(response->consistencias[i], consistencia_string);
			strcat(temporal, consistencia_string);
			strcat(temporal, "\n");
			strcat(temporal, "Numero de Particiones: ");
			obtener_key_string(response->numeros_particiones[i],
					n_particiones_string);
			strcat(temporal, n_particiones_string);
			strcat(temporal, "\n");
			strcat(temporal, "Tiempo entre Compactaciones: ");
			obtener_key_string(response->tiempos_compactaciones[i],
					t_compactacion_string);
			strcat(temporal, t_compactacion_string);
			strcat(temporal, "\n\n");

			consistencia_string[0] = '\0';
			n_particiones_string[0] = '\0';
			t_compactacion_string[0] = '\0';
			free(tablas_string_lista[i]);
		}
		free(tablas_string_lista);
	}
	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int drop_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct drop_response *response = (struct drop_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];

	if (response->fallo) {
		strcpy(temporal, "\nFallo DROP\n");
	} else {
		strcpy(temporal, "\nResultado DROP\n");
		strcat(temporal, "Tabla: ");
		strcat(temporal, response->tabla);
		strcat(temporal, "\n");
	}
	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int journal_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct journal_response *response = (struct journal_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	if (response->fallo) {
		strcpy(temporal, "\nFallo JOURNAL\n");
	} else {
		strcpy(temporal, "\nJOURNAL realizado\n");
	}
	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int add_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct add_response *response = (struct add_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	char n_memoria_string[TAMANIO_MAX_INT_STRING];
	char criterio_string[TAMANIO_MAX_INT_STRING];

	if (response->fallo) {
		strcpy(temporal, "\nFallo ADD\n");
	} else {
		strcpy(temporal, "\nResultado ADD\n");
		strcat(temporal, "Numero de Memoria: ");
		obtener_key_string(response->n_memoria, n_memoria_string);
		strcat(temporal, n_memoria_string);
		strcat(temporal, "\n");
		strcat(temporal, "Criterio: ");
		obtener_key_string(response->criterio, criterio_string);
		strcat(temporal, criterio_string);
		strcat(temporal, "\n");
	}
	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int run_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct run_response *response = (struct run_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];

	if (response->fallo) {
		strcpy(temporal, "\nFallo RUN\n");
	} else {
		strcpy(temporal, "\nResultado RUN\n");
		strcat(temporal, "Direccion archivo LQL: ");
		strcat(temporal, response->path);
		strcat(temporal, "\n");
	}
	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int metrics_response_to_string(void* mensaje, char* buffer, int tamanio_maximo) {
	struct metrics_response *response = (struct metrics_response*) mensaje;
	char temporal[TAMANIO_MAX_STRING];
	if (response->fallo) {
		strcpy(temporal, "\nFallo METRICS\n");
	} else {
		strcpy(temporal, "\nResultado METRICS\n");
		strcat(temporal, response->resultado);
		strcat(temporal, "\n");
	}
	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int error_msg_a_string(void *mensaje, char *buffer, int tamanio_maximo) {
	struct error_msg *response = (struct error_msg*) mensaje;
	char temporal[TAMANIO_MAX_STRING] = { 0 };
	sprintf(temporal, "ERROR: %s\n", response->description);
	if (tamanio_maximo < strlen(temporal) + 1) {
		return -1;
	}
	strcpy(buffer, temporal);
	return 0;
}

int memory_full_a_string(void *mensaje, char *buffer, int tamanio_maximo) {
	if(tamanio_maximo < 14) {
		return -1;
	}
	strcpy(buffer, "Memoria llena");
	return 0;
}

void cargar_convertidor(uint8_t id, convertidor_t convertidor) {
	char id_string[4];
	obtener_key_string(id, id_string);
	dictionary_put(diccionario, id_string, convertidor);
}

void llenar_diccionario() {
	cargar_convertidor(SELECT_RESPONSE_ID, &select_response_to_string);
	cargar_convertidor(INSERT_RESPONSE_ID, &insert_response_to_string);
	cargar_convertidor(CREATE_RESPONSE_ID, &create_response_to_string);
	cargar_convertidor(SINGLE_DESCRIBE_RESPONSE_ID,
			&single_describe_response_to_string);
	cargar_convertidor(GLOBAL_DESCRIBE_RESPONSE_ID,
			&global_describe_response_to_string);
	cargar_convertidor(DROP_RESPONSE_ID, &drop_response_to_string);
	cargar_convertidor(JOURNAL_RESPONSE_ID, &journal_response_to_string);
	cargar_convertidor(ADD_RESPONSE_ID, &add_response_to_string);
	cargar_convertidor(RUN_RESPONSE_ID, &run_response_to_string);
	cargar_convertidor(METRICS_RESPONSE_ID, &metrics_response_to_string);
	cargar_convertidor(ERROR_MSG_ID, &error_msg_a_string);
	cargar_convertidor(MEMORY_FULL_ID, &memory_full_a_string);
}

void iniciar_diccionario() {
	diccionario = dictionary_create();
	llenar_diccionario();
}

void cerrar_diccionario() {
	dictionary_clean(diccionario);
	dictionary_destroy(diccionario);
}
