#ifndef PARSER_H_
#define PARSER_H_

#define MAX_TOKENS_LENGTH    100

enum errores{ VALUE_INVALIDO = -7,
			  CONSISTENCIA_INVALIDA,
	          ERROR_TAMANIO_BUFFER,
			  IDENTIFICADOR_INVALIDO,
			  CONSTANTE_INVALIDA,
			  COMANDOS_INVALIDOS,
			  ERROR,
			  FALSE,
			  OK
};

int parser(char*,void*,int);
int manejarError(int, char*,int);

#endif /* PARSER_H_ */
