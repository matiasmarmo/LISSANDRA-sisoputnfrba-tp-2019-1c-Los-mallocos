#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
#include "test-main.h"
#include "test-parser.c"
#include "test-consola.c"

void AddTests(int*,int);
int validarEntrada(int, char*[], int*);

int main(int argc, char* argv[]){
	int testsACorrer[20] = {};
	if(validarEntrada(argc,argv,testsACorrer)==-1){
		printf("ERROR: Alguno de los tests solicitados no existe\n");
		return 0;
	}
	CU_BasicRunMode mode = CU_BRM_VERBOSE;
	CU_ErrorAction error_action = CUEA_IGNORE;

	setvbuf(stdout, NULL, _IONBF, 0);

	if(CU_initialize_registry()){
		fprintf(stderr, " Initialization of Test Registry failed. ");
		exit(1);
	}else{
		AddTests(testsACorrer,argc-1);
		CU_basic_set_mode(mode);
		CU_set_error_action(error_action);
		printf("\nTests completed with return value %d.\n", CU_basic_run_tests());
		CU_cleanup_registry();
	}
	return EXIT_SUCCESS;
}
/*----------Setting environment----------------*/
void AddTests(int testsACorrer[], int tamanio)
{
	assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());

    for(int i=0;i<tamanio;i++){
    	switch(testsACorrer[i]){
    		case Parser:
    			if(CUE_SUCCESS != CU_register_suites(suites_parser)){
    				fprintf(stderr, "Register suites failed - %s ", CU_get_error_msg());
    				exit(1);
    			}
    			break;
    		case Consola:
    			if(CUE_SUCCESS != CU_register_suites(suites_consola)){
    			   	fprintf(stderr, "Register suites failed - %s ", CU_get_error_msg());
    			   	exit(1);
    			}
    			break;
    		case All:
    			tamanio = CANT_TESTS;
    			i=-1;
    			for(int cont=1; cont<=CANT_TESTS;cont++){ testsACorrer[cont-1] = cont; }
    			break;
    	}
    }
}
int validarEntrada(int argc, char* argv[], int* testsACorrer){
	for(int i=1;i<argc;i++){
		  	if(!strcmp(argv[i],"All")){
		    	testsACorrer[i-1] = All;
		   	}else if(!strcmp(argv[i],"Parser")){
		 		testsACorrer[i-1] = Parser;
		  	}else if(!strcmp(argv[i],"Consola")){
		 		testsACorrer[i-1] = Consola;
		  	}else{
		  		return -1;
		  	}
	}
	return EXIT_SUCCESS;
}
