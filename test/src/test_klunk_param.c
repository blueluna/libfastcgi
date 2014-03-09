#include <stdint.h>
#include <assert.h>
#include <stdio.h>

#include "testcase.h"
#include "klunk_param.h"
#include "errorcodes.h"
#include "llist.h"
#include "test_klunk_param.h"

void klunk_param_test()
{
	const char* name = "lorem ipsum";
	const size_t name_len = strlen(name);
	const char* value = "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
	const size_t value_len = strlen(value);
	int str_result = 0;
	klunk_param_t* param = 0;

	param = klunk_param_create(name, name_len, value, value_len);
	TEST_ASSERT_NOT_EQUAL(param, 0);
	if (param != 0) {
		str_result = strlen(param->name);
		TEST_ASSERT_EQUAL(str_result, (int)name_len);
		
		str_result = strcmp(name, param->name);
		TEST_ASSERT_EQUAL(str_result, 0);

		str_result = strlen(param->value);
		TEST_ASSERT_EQUAL(str_result, (int)value_len);
		
		str_result = strcmp(value, param->value);
		TEST_ASSERT_EQUAL(str_result, 0);
		
		klunk_param_destroy(param);
	}
	param = klunk_param_create(value, value_len, name, name_len);
	TEST_ASSERT_NOT_EQUAL(param, 0);
	if (param != 0) {
		str_result = strlen(param->name);
		TEST_ASSERT_EQUAL(str_result, (int)value_len);

		str_result = strcmp(value, param->name);
		TEST_ASSERT_EQUAL(str_result, 0);

		str_result = strlen(param->value);
		TEST_ASSERT_EQUAL(str_result, (int)name_len);
		
		str_result = strcmp(name, param->value);
		TEST_ASSERT_EQUAL(str_result, 0);
		
		klunk_param_destroy(param);
	}
}

void klunk_param_llist_test()
{
	int32_t result = E_SUCCESS;
	char* name;
	size_t name_len;
	char* value;
	size_t value_len;
	int n = 0;
	int str_result = 0;
	klunk_param_t* param = 0;
	llist_t* list = 0; 
	llist_item_t *ll_item = 0;

	name = malloc(256);
	assert(name != 0);
	value = malloc(256);
	assert(value != 0);
	list = llist_create(sizeof(klunk_param_t));
	assert(list != 0);
	result = llist_register_dtor(list, klunk_param_destroy);
	assert(result == E_SUCCESS);
	TEST_ASSERT_EQUAL(result, E_SUCCESS);

	for (n = 0; n < 10; n++) {
		name_len = snprintf(name, 256, "item %d", n);
		value_len = snprintf(value, 256, "value %d", n);
		param = klunk_param_create(name, name_len, value, value_len);
		assert(param != 0);
		llist_add(list, param, sizeof(klunk_param_t));
	}

	ll_item = llist_begin(list);
	for (n = 0; n < 10; n++) {
		name_len = snprintf(name, 256, "item %d", n);
		value_len = snprintf(value, 256, "value %d", n);
		param = (klunk_param_t*)ll_item->data;

		str_result = strlen(param->name);
		TEST_ASSERT_EQUAL(str_result, (int)name_len);

		str_result = strcmp(name, param->name);
		TEST_ASSERT_EQUAL(str_result, 0);

		str_result = strlen(param->value);
		TEST_ASSERT_EQUAL(str_result, (int)value_len);
		
		str_result = strcmp(value, param->value);
		TEST_ASSERT_EQUAL(str_result, 0);

		ll_item = ll_item->next;
	}

	ll_item = llist_begin(list);

	llist_remove(list, ll_item->data);

	ll_item = llist_begin(list);
	for (n = 1; n < 10; n++) {
		name_len = snprintf(name, 256, "item %d", n);
		value_len = snprintf(value, 256, "value %d", n);
		param = (klunk_param_t*)ll_item->data;

		str_result = strlen(param->name);
		TEST_ASSERT_EQUAL(str_result, (int)name_len);

		str_result = strcmp(name, param->name);
		TEST_ASSERT_EQUAL(str_result, 0);

		str_result = strlen(param->value);
		TEST_ASSERT_EQUAL(str_result, (int)value_len);
		
		str_result = strcmp(value, param->value);
		TEST_ASSERT_EQUAL(str_result, 0);

		ll_item = ll_item->next;
	}

	llist_destroy(list);
	free(name);
	free(value);
}
