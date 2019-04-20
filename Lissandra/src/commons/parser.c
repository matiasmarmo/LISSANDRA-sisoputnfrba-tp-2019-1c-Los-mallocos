#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include "parser.h"
#include "comunicacion/protocol.h"

int isIdentifier(char* );
int isConstant(char* );
int obtenerProximaPalabra(char* , char* ,char ,int );
int ejecutarSelect(char*,char*,int);

int parser(char* linea,void* msg, int tamanioBuffer){
	if(!memcmp(linea,"SELECT ",7      )){ return ejecutarSelect(linea,msg,tamanioBuffer);}
	return ERROR;
}

int obtenerProximaPalabra(char* msg, char* buff,char centinela,int inicio){
	int contador=inicio;
	while(msg[contador] != centinela && msg[contador] != '\0'){
		contador++;
	}
	if(msg[contador] == '\0' && centinela != '\0') {return -1;}
	int letra;
	for(letra=inicio; letra<contador; letra++){
		buff[letra-inicio] = msg[letra];
	}
	buff[letra-inicio] = '\0';
	return contador-inicio;
}

int ejecutarSelect(char* msg,char* buffer, int tamanioBuffer){
	//SELECT + NOMBRE_TABLA + KEY
	int inicio = 7;
	char nombreTabla[MAX_TOKENS_LENGTH];
	char key[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	tamanioPalabra = obtenerProximaPalabra(msg,key, '\0', inicio+tamanioPalabra+1);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(key)){ return CONSTANTE_INVALIDA; }

	uint64_t maximo = strtoumax(key,NULL,10);
	if(maximo > UINT16_MAX){ return CONSTANTE_INVALIDA; }
	uint16_t nuevaKey = strtoumax(key,NULL,10);

	struct select_request mensaje;
	init_select_request(nombreTabla,nuevaKey,&mensaje);
	if(sizeof(struct select_request) > tamanioBuffer){ return ERROR_TAMANIO_BUFFER; }
	memcpy(buffer, &mensaje, sizeof(struct select_request));
	return OK;
}

int isIdentifier(char* id){ //Controla que id este conformado solo por letras , '_' o '-'
	for(int i=0 ; i < strlen(id); i++){
		if(!( isalnum(id[i]) || id[i]=='_' || id[i]=='-')){
			return FALSE;
		}
	}
	return OK;
}
int isConstant(char* id){ //Controla que id este conformado solo por numeros
	for(int i=0 ; i < strlen(id); i++){
		if(!isdigit(id[i])){
			return FALSE;
		}
	}
	return OK;
}
