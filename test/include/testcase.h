#ifndef ES_TESTCASE_H
#define ES_TESTCASE_H

#include <stdlib.h>
#include <stdio.h>

void test_assert_true(int boolean, const char* expr, const char* filename, const int line);

#define TEST_ASSERT_TRUE(expression) \
	test_assert_true(expression, #expression, __FILE__, __LINE__)

#define TEST_ASSERT_EQUAL(a, b) \
	TEST_ASSERT_TRUE(a == b)

#define TEST_ASSERT_NOT_EQUAL(a, b) \
	TEST_ASSERT_TRUE(a != b)

#endif /* ES_TESTCASE_H */
