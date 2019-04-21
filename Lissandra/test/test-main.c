#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include "../src/commons/comunicacion/protocol.h"
#include "../src/commons/parser.h"
#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"

void AddTests(void);
int main()
{
	CU_BasicRunMode mode = CU_BRM_VERBOSE;
	CU_ErrorAction error_action = CUEA_IGNORE;

    setvbuf(stdout, NULL, _IONBF, 0);

    if(CU_initialize_registry()){
    	fprintf(stderr, " Initialization of Test Registry failed. ");
        exit(1);
    }else{
        AddTests();
        CU_basic_set_mode(mode);
        CU_set_error_action(error_action);
        printf("\nTests completed with return value %d.\n", CU_basic_run_tests());
        CU_cleanup_registry();
    }
    return 0;
}
/*-------- Functions to be tested ---------------*/
extern int parser(char*,void*,int);

/*-------- Test cases ------------------*/
void test_1(){
		//...
}

CU_TestInfo testcases[] = {
		// {"Testing .... is ok:", test_1},
        // {"Testing .... is wrong:", test_1},
        // ...
		CU_TEST_INFO_NULL
};

/*--------------- Test suites ------------------*/
int suite_success_init(void) { return 0; }
int suite_success_clean(void) { return 0; }

CU_SuiteInfo suites[] = {
        {"Testing the function XXXX:", suite_success_init, suite_success_clean, NULL, NULL, testcases},
		// ...
		CU_SUITE_INFO_NULL
};
/*-------- Setting enviroment ------------------*/
void AddTests(void)
{
	assert(NULL != CU_get_registry());
    assert(!CU_is_test_running());

    if(CUE_SUCCESS != CU_register_suites(suites)){
    	fprintf(stderr, "Register suites failed - %s ", CU_get_error_msg());
        exit(1);
    }
}


