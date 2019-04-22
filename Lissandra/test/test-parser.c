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
		char* msg = "INSERT TABLA1 3 Mi nombre es Lissandra\" 1548421507";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),VALUE_INVALIDO);
}
void test_insert_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "INSERT TABLA1 B3 \"Mi nombre es Lissandra\" 1548421507";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSTANTE_INVALIDA);
}
/*-----------------Test Cases--------------------*/
CU_TestInfo testcasesSelect[] = {
		{"Testing SELECT tabla1 123 is ok:", test_select_1},
        {"Testing SELECT tabla1 123456789101121212 is wrong:", test_select_2},
        {"Testing SELECT is wrong:", test_select_3},
		{"Testing SELECT tabla1 is wrong:", test_select_4},
		{"Testing SELECT ab!cd is wrong:", test_select_5},
		{"Testing SELECT<space> is wrong:", test_select_6},
		{"Testing SELECT tabla1 123 asdf is wrong:", test_select_7},
		{"Testing SELECT tabla1 12c3 is wrong:", test_select_8},
		{"Testing SELECT ab!c 123 is wrong:", test_select_9},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesInsert[] = {
		{"Testing INSERT TABLA1 3 \"Mi nombre es Lissandra\" 1548421507 is ok:", test_insert_1},
        {"Testing INSERT TABLA1 3 Mi nombre es Lissandra\" 1548421507 is wrong:", test_insert_2},
        {"Testing INSERT TABLA1 B3 \"Mi nombre es Lissandra\" 1548421507 is wrong:", test_insert_3},
		CU_TEST_INFO_NULL
};
/*------------- Suites -----------------------*/
CU_SuiteInfo suites_parser[] = {
        {"Testing the function parser, SELECT command:", suite_success_init, suite_success_clean, NULL, NULL, testcasesSelect},
		{"Testing the function parser, INSERT command:", suite_success_init, suite_success_clean, NULL, NULL, testcasesInsert},
		CU_SUITE_INFO_NULL
};



