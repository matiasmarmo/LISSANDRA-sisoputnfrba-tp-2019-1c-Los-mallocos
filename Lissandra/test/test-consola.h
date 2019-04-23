#ifndef TEST_CONSOLA_H_
#define TEST_CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "../src/commons/comunicacion/protocol.h"
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

void test_1();
void test_2();
/*-----------------Test Cases--------------------*/
CU_TestInfo testcases1[] = {
		//.........
		CU_TEST_INFO_NULL
};
CU_TestInfo testcases2[] = {
		//.........
		CU_TEST_INFO_NULL
};

/*------------- Suites -----------------------*/
CU_SuiteInfo suites_consola[] = {
        //.........
		CU_SUITE_INFO_NULL
};

#endif /* TEST_CONSOLA_H_ */
