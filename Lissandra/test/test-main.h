#ifndef TEST_MAIN_H_
#define TEST_MAIN_H_

#define CANT_TESTS  2

enum tests{ All,
			Parser,
			Consola
};
/*------------- Test suites ------------------*/
int suite_success_init(void) { return 0; }
int suite_success_clean(void) { return 0; }

#endif /* TEST_MAIN_H_ */
