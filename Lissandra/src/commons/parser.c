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
int ejecutarInsert(char*,char*,int);
int ejecutarCreate(char*,char*,int);
int ejecutarDescribe(char*,char*,int);


int parser(char* linea,void* msg, int tamanioBuffer){
	if(!memcmp(linea,"SELECT ",7      )){ return ejecutarSelect(linea,msg,tamanioBuffer);}
	if(!memcmp(linea,"INSERT ",7      )){ return ejecutarInsert(linea,msg,tamanioBuffer);}
	if(!memcmp(linea,"CREATE ",7      )){ return ejecutarCreate(linea,msg,tamanioBuffer);}
	if(!memcmp(linea,"DESCRIBE",8     )){ return ejecutarDescribe(linea,msg,tamanioBuffer);}
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

int ejecutarInsert(char* msg,char* buffer, int tamanioBuffer){
	//INSERT + NOMBRE_TABLA + KEY + "VALUE" + TIMESTAMP
	int inicio = 7;
	char nombreTabla[MAX_TOKENS_LENGTH];
	char key[MAX_TOKENS_LENGTH];
	char value[MAX_TOKENS_LENGTH];
	char espacio[MAX_TOKENS_LENGTH];
	char timestamp[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,key, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(key)){ return CONSTANTE_INVALIDA; }

	uint64_t maximo = strtoumax(key,NULL,10);
	if(maximo > UINT16_MAX){ return CONSTANTE_INVALIDA; }
	uint16_t nuevaKey = strtoumax(key,NULL,10);

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,espacio, '"', inicio);
	if(tamanioPalabra!= 0){ return VALUE_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg, value, '"', inicio);

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,espacio, ' ', inicio);
	if(tamanioPalabra!= 0){ return VALUE_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,timestamp, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(timestamp)){ return CONSTANTE_INVALIDA; }
	uint64_t nuevoTimestamp = strtoumax(timestamp,NULL,10);

	struct insert_request mensaje;
	init_insert_request(nombreTabla,nuevaKey,value,nuevoTimestamp,&mensaje);
	if(sizeof(struct insert_request) > tamanioBuffer){ return ERROR_TAMANIO_BUFFER; }
	memcpy(buffer, &mensaje, sizeof(struct insert_request));
	return OK;
}
int ejecutarCreate(char* msg,char* buffer, int tamanioBuffer){
	//CREATE + NOMBRE_TABLA + TIPO CONSISTENCIA + NUMERO PARTICIONES + TIEMPO DE COMPACTACION
	int inicio = 7;
	char nombreTabla[MAX_TOKENS_LENGTH];
	char consistencia[MAX_TOKENS_LENGTH];
	char particiones[MAX_TOKENS_LENGTH];
	char compactacion[MAX_TOKENS_LENGTH];

	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,consistencia, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	int valorConsistencia = isConsistency(consistencia);
	if(valorConsistencia < 0){ return CONSISTENCIA_INVALIDA; }

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,particiones, ' ', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(particiones)){ return CONSTANTE_INVALIDA; };
	uint64_t maximo = strtoumax(particiones,NULL,10);
	if(maximo > UINT16_MAX){ return CONSTANTE_INVALIDA; }
	uint8_t nuevaParticion = strtoumax(particiones,NULL,10);

	inicio=inicio+tamanioPalabra+1;
	tamanioPalabra = obtenerProximaPalabra(msg,compactacion, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isConstant(compactacion)){ return CONSTANTE_INVALIDA; }

	uint64_t maximo2 = strtoumax(compactacion,NULL,10);
	if(maximo2 > UINT16_MAX){ return CONSTANTE_INVALIDA; }
	uint32_t nuevaCompactacion = strtoumax(compactacion,NULL,10);

	struct create_request mensaje;
	init_create_request(nombreTabla,valorConsistencia,nuevaParticion,nuevaCompactacion,&mensaje);
	if(sizeof(struct create_request) > tamanioBuffer){ return ERROR_TAMANIO_BUFFER; }
	memcpy(buffer, &mensaje, sizeof(struct create_request));
	return OK;
}

int ejecutarDescribe(char* msg,char* buffer, int tamanioBuffer){
	//DESCRIBE + NOMBRE_TABLA o DESCRIBE
	int inicio = 8;
	char nombreTabla[MAX_TOKENS_LENGTH];
	if(msg[inicio]=='\0'){
		struct describe_request mensaje;
		init_describe_request(0,NULL,&mensaje);
		memcpy(buffer, &mensaje, sizeof(struct describe_request));
		return OK;
	}
	inicio += 1;
	int tamanioPalabra = obtenerProximaPalabra(msg, nombreTabla, '\0', inicio);
	if(tamanioPalabra == -1){ return COMANDOS_INVALIDOS; }
	if(!isIdentifier(nombreTabla)){ return IDENTIFICADOR_INVALIDO; }

	struct describe_request mensaje;
	init_describe_request(1,nombreTabla,&mensaje);
	if(sizeof(struct describe_request) > tamanioBuffer){ return ERROR_TAMANIO_BUFFER; }
	memcpy(buffer, &mensaje, sizeof(struct describe_request));
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
