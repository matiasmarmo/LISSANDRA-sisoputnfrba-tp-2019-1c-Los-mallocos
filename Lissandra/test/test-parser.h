#ifndef TEST_PARSER_H_
#define TEST_PARSER_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "../src/commons/comunicacion/protocol.h"
#include "../src/commons/parser.h"
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

void test_select_1();
void test_select_2();
void test_select_3();
void test_select_4();
void test_select_5();
void test_select_6();
void test_select_7();
void test_select_8();
void test_select_9();
void test_insert_1();
void test_insert_2();
void test_insert_3();
void test_insert_4();
void test_insert_5();
void test_insert_6();
void test_run_1();
void test_run_2();
void test_run_3();
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
		{"Testing INSERT TABLA1 3 \"Mi nombre es Lissandra\" is ok:", test_insert_2},
        {"Testing INSERT TABLA1 3 Mi nombre es Lissandra\" 1548421507 is wrong:", test_insert_3},
        {"Testing INSERT TABLA1 B3 \"Mi nombre es Lissandra\" 1548421507 is wrong:", test_insert_4},
		{"Testing INSERT TABLA1 3 \"Mi nombre es Lissandra\" B1507 is wrong", test_insert_5},
		{"Testing INSERT TABLA1 3 \"Mi nombre es Lissandra\"  1548421507 is wrong", test_insert_6},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesRun[] = {
		{"Testing RUN /usr/nico/home/nombre-archivo is ok:", test_run_1},
		{"Testing RUN /usr/nico/home /nombre-archivo is wrong:", test_run_2},
        {"Testing RUN &/usr/nico/home/nombre-archivo is wrong:", test_run_3},
		CU_TEST_INFO_NULL
};
/*------------- Suites -----------------------*/
CU_SuiteInfo suites_parser[] = {
        {"Testing the function parser, SELECT command:", 0, 0, NULL, NULL, testcasesSelect},
		{"Testing the function parser, INSERT command:", 0, 0, NULL, NULL, testcasesInsert},
		{"Testing the function parser, RUN command:", 0, 0, NULL, NULL, testcasesRun},
		CU_SUITE_INFO_NULL
};





#endif /* TEST_PARSER_H_ */
