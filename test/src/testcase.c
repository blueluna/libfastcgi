#include "testcase.h"

void test_assert_true(int boolean, const char* expr, const char* filename, const int line)
{
	if (!boolean) {
		printf("Test %s failed at %s:%d\n", expr, filename, line);
	}
}
