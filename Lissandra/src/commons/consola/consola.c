#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../comunicacion/protocol.h"
#include "../parser.h"
#include "consola.h"
#include "mensaje-a-string.h"

handler_t handler_actual;

pthread_mutex_t consola_mutex = PTHREAD_MUTEX_INITIALIZER;

void imprimir(char* linea) {
	if(pthread_mutex_lock(&consola_mutex) != 0) {
		return;
	}
	rl_save_prompt();
	rl_redisplay();
	printf("%s\n", linea);
	rl_restore_prompt();
	pthread_mutex_unlock(&consola_mutex);
}

void imprimir_async(char* linea) {
	if(pthread_mutex_lock(&consola_mutex) != 0) {
		return;
	}
	char *linea_guardada;
	int punto = rl_point;
	linea_guardada = rl_copy_text(0, rl_end);
	rl_save_prompt();
	rl_replace_line("", 0);
	rl_redisplay();
	printf("%s\n", linea);
	rl_restore_prompt();
	rl_replace_line(linea_guardada, 0);
	rl_point = punto;
	rl_redisplay();
	pthread_mutex_unlock(&consola_mutex);
	free(linea_guardada);
}

void ejecutar_nueva_linea(char *linea) {
	if (linea == NULL) {
		free(linea);
		cerrar_consola();
		return;
	}
	add_history(linea);
	int tamanio_maximo_buffer = get_max_msg_size();
	uint8_t mensaje[tamanio_maximo_buffer];

	int respuesta_parser = parser(linea, mensaje, tamanio_maximo_buffer);
	if(respuesta_parser < 0){
		char buffer_parser_error[TAMANIO_MAX_STRING];
		manejarError(respuesta_parser,buffer_parser_error, TAMANIO_MAX_STRING);
		imprimir(buffer_parser_error);
		free(linea);
		return;
	}

	if (handler_actual != NULL) {
		handler_actual(linea, mensaje);
	}
	free(linea);
}

void iniciar_consola(handler_t handler) {
	handler_actual = handler;
	rl_callback_handler_install("> ", ejecutar_nueva_linea);
	iniciar_diccionario();
}

void leer_siguiente_caracter() {
	rl_callback_read_char();
}

void mostrar(void* mensaje) {
	char buffer[TAMANIO_MAX_STRING];
	convertir_en_string(mensaje, buffer, TAMANIO_MAX_STRING);
	imprimir(buffer);
}

void mostrar_async(void* mensaje) {
	char buffer[TAMANIO_MAX_STRING];
	convertir_en_string(mensaje, buffer, TAMANIO_MAX_STRING);
	imprimir_async(buffer);
}


void cerrar_consola() {
	rl_callback_handler_remove();
	rl_clear_history();
	cerrar_diccionario();
}
