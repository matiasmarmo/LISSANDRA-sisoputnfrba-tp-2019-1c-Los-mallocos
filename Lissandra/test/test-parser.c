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
void test_create_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "CREATE TABLA1 SC 4 60000";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_create_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "CREATE TABLA1 EC 4 60000";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_create_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "CREATE TABLA1 acereje 4 60000";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR_TAMANIO_BUFFER);
}
void test_create_4(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "CREATE TABLA1 SC B4 60000";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),COMANDOS_INVALIDOS);
}
void test_create_5(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "CREATE TABLA1 SpC 4";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR_TAMANIO_BUFFER);
}
void test_create_6(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "CREATE";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR);
}
void test_describe_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DESCRIBE TABLA1";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_describe_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DESCRIBE";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_describe_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DESCRIBE TABLA1 acereje";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),IDENTIFICADOR_INVALIDO);
}
void test_describe_4(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DESCRIBE TABLA#$";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),IDENTIFICADOR_INVALIDO);
}
void test_drop_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DROP TABLA1";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_drop_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DROP";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR);
}
void test_drop_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DROP TABLA1 acereje";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),IDENTIFICADOR_INVALIDO);
}
void test_drop_4(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "DROP TABLA#$";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),IDENTIFICADOR_INVALIDO);
}
void test_journal_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "JOURNAL";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_journal_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "JOURNAL TABLA1";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR);
}
void test_journal_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "JOURNAL TABLA1 acereje";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR);
}
void test_journal_4(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "JOURNAL TABLA1 acereje";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR);
}
void test_add_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "ADD MEMORY 4 TO SC";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_add_2(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "ADD MEMORY 400 TO EC";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_add_3(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "ADD MEMORY acereje TO EC";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),IDENTIFICADOR_INVALIDO);
}
void test_add_4(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "ADD MEMORY 400 TO 22";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),CONSISTENCIA_INVALIDA);
}
void test_add_5(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "ADD MEMORY";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),ERROR);
}
void test_add_6(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "ADD MEMORY 400 acereje SC";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),IDENTIFICADOR_INVALIDO);
}
void test_metrics_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "METRICS";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
}
void test_exit_1(){
		uint8_t mensaje[get_max_msg_size()];
		int a= get_max_msg_size();
		char* msg = "EXIT";
        CU_ASSERT_EQUAL(parser(msg,mensaje,a),OK);
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
