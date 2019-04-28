#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "../src/commons/comunicacion/protocol.h"
#include "../src/commons/parser.h"
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
/*-----------------Function to be tested---------*/
extern int parser(char*,void*,int);
/*-------- Test cases <SELECT>------------------*/
void test_select_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT tabla1 123";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_select_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT tabla1 123456789101121212";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSTANTE_INVALIDA);
}
void test_select_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR);
}
void test_select_4(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT tabla1";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),COMANDOS_INVALIDOS);
}
void test_select_5(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT ab!cd";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),COMANDOS_INVALIDOS);
}
void test_select_6(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT ";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),COMANDOS_INVALIDOS);
}
void test_select_7(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT tabla1 123 asdf";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSTANTE_INVALIDA);
}
void test_select_8(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT tabla1 12c3";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSTANTE_INVALIDA);
}
void test_select_9(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "SELECT ab!c 123";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),IDENTIFICADOR_INVALIDO);
}
/*-------- test cases <INSERT>------------------*/
void test_insert_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "INSERT TABLA1 3 \"Mi nombre es Lissandra\" 1548421507";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_insert_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "INSERT TABLA1 3 \"Mi nombre es Lissandra\"";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_insert_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "INSERT TABLA1 3 Mi nombre es Lissandra\" 1548421507";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),VALUE_INVALIDO);
}
void test_insert_4(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "INSERT TABLA1 B3 \"Mi nombre es Lissandra\" 1548421507";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSTANTE_INVALIDA);
}
void test_insert_5(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "INSERT TABLA1 3 \"Mi nombre es Lissandra\" BBs1507";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSTANTE_INVALIDA);
}
void test_insert_6(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "INSERT TABLA1 3 \"Mi nombre es Lissandra\"  1548421507";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSTANTE_INVALIDA);
}
void test_run_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "RUN /usr/nico/home/nombre-archivo";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_run_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "RUN /usr/nico/home /nombre-archivo";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),DIRECCION_INVALIDA);
}
void test_run_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "RUN &/usr/nico/home/nombre-archivo";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),DIRECCION_INVALIDA);
}
