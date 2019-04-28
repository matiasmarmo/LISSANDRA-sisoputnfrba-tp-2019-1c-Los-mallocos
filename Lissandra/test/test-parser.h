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
void test_create_1();
void test_create_2();
void test_create_3();
void test_create_4();
void test_create_5();
void test_create_6();
void test_describe_1();
void test_describe_2();
void test_describe_3();
void test_describe_4();
void test_drop_1();
void test_drop_2();
void test_drop_3();
void test_drop_4();
void test_journal_1();
void test_journal_2();
void test_journal_3();
void test_journal_4();
void test_add_1();
void test_add_2();
void test_add_3();
void test_add_4();
void test_add_5();
void test_add_6();
void test_metrics_1();
void test_exit_1();
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
CU_TestInfo testcasesCreate[] = {
		{"Testing CREATE TABLA1 SC 4 60000 is ok", test_insert_1},
		{"Testing CREATE TABLA1 EC 4 60000 is ok", test_insert_2},
        {"Testing CREATE TABLA1 acereje 4 60000 is wrong", test_insert_3},
        {"Testing CREATE TABLA1 SC B4 60000 is wrong", test_insert_4},
		{"Testing CREATE TABLA1 SpC 4 is wrong", test_insert_5},
		{"Testing CREATE is wrong", test_insert_6},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesDescribe[] = {
		{"Testing DESCRIBE TABLA1 is ok", test_describe_1},
		{"Testing DESCRIBE is ok", test_describe_2},
        {"Testing DESCRIBE TABLA1 acereje is wrong", test_describe_3},
        {"Testing DESCRIBE TABLA#$ is wrong", test_describe_4},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesDrop[] = {
		{"Testing DROP TABLA1 is ok", test_drop_1},
		{"Testing DROP is wrong", test_drop_2},
        {"Testing DROP TABLA1 acereje is wrong", test_drop_3},
        {"Testing DROP TABLA#$ is wrong", test_drop_4},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesJournal[] = {
		{"Testing JOURNAL is ok:", test_run_1},
		{"Testing JOURNAL TABLA1 is wrong:", test_run_2},
        {"Testing JOURNAL TABLA1 acereje is wrong:", test_run_3},
		{"Testing JOURNAL TABLA1 acereje is wrong:", test_journal_4},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesAdd[] = {
		{"Testing ADD MEMORY 4 TO SC is ok", test_add_1},
		{"Testing ADD MEMORY 400 TO EC is ok", test_add_2},
        {"Testing ADD MEMORY acereje TO EC is wrong", test_add_3},
        {"Testing ADD MEMORY 400 TO 22 is wrong", test_add_4},
		{"Testing ADD MEMORY is wrong", test_add_5},
		{"Testing ADD MEMORY 400 acereje SC is wrong", test_add_6},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesMetrics[] = {
		{"Testing METRICS is ok:", test_metrics_1},
		CU_TEST_INFO_NULL
};
CU_TestInfo testcasesExit[] = {
		{"Testing EXIT is ok:", test_exit_1},
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
		{"Testing the function parser, CREATE command:", 0, 0, NULL, NULL, testcasesCreate},
		{"Testing the function parser, DESCRIBE command:", 0, 0, NULL, NULL, testcasesDescribe},
		{"Testing the function parser, DROP command:", 0, 0, NULL, NULL, testcasesDrop},
		{"Testing the function parser, JOURNAL command:", 0, 0, NULL, NULL, testcasesJournal},
		{"Testing the function parser, ADD command:", 0, 0, NULL, NULL, testcasesAdd},
		{"Testing the function parser, METRICS command:", 0, 0, NULL, NULL, testcasesMetrics},
		{"Testing the function parser, EXIT command:", 0, 0, NULL, NULL, testcasesExit},
		{"Testing the function parser, RUN command:", 0, 0, NULL, NULL, testcasesRun},
		CU_SUITE_INFO_NULL
};





#endif /* TEST_PARSER_H_ */
