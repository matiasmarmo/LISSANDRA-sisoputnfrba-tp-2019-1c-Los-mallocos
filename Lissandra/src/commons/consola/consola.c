#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../comunicacion/protocol.h"
#include "../parser.h"
#include "consola.h"
#include "mensaje-a-string.h"

handler_t handler_actual;

void imprimir(char* linea) {
	rl_save_prompt();
	rl_redisplay();
	printf("%s\n", linea);
	rl_restore_prompt();
	rl_redisplay();
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
	}

	if (handler_actual != NULL) {
		handler_actual(mensaje);
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

void cerrar_consola() {
	rl_callback_handler_remove();
	rl_clear_history();
	cerrar_diccionario();
}
