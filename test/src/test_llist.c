#include "testcase.h"
#include "llist.h"
#include "errorcodes.h"
#include "test_llist.h"

void llist_test()
{
	int32_t result = 0;
	llist_t* list = llist_create(sizeof(struct item));
	llist_item_t *ll_item = 0;
	struct item *an_item;
	TEST_ASSERT_NOT_EQUAL(list, 0);
	if (list != 0) {
		ll_item = llist_begin(list);
		TEST_ASSERT_EQUAL(ll_item, 0);
		
		struct item *i1 = malloc(sizeof(struct item));
		i1->number = 1;
		i1->character = '1';
		
		result = llist_add(list, i1, sizeof(struct item));
		TEST_ASSERT_EQUAL(result, E_SUCCESS);

		ll_item = llist_begin(list);
		TEST_ASSERT_NOT_EQUAL(ll_item, 0);
		TEST_ASSERT_EQUAL(ll_item->data, i1);
		an_item = (struct item*)(ll_item->data);
		TEST_ASSERT_EQUAL(an_item->number, 1);
		TEST_ASSERT_EQUAL(an_item->character, '1');
		
		result = llist_add(list, i1, sizeof(struct item));
		TEST_ASSERT_EQUAL(result, E_DUPLICATE);

		ll_item = llist_find(list, i1);
		TEST_ASSERT_NOT_EQUAL(ll_item, 0);
		TEST_ASSERT_EQUAL(ll_item->data, i1);
		an_item = (struct item*)(ll_item->data);
		TEST_ASSERT_EQUAL(an_item->number, 1);
		TEST_ASSERT_EQUAL(an_item->character, '1');
		
		struct item *i2 = malloc(sizeof(struct item));
		i2->number = 2;
		i2->character = '2';

		result = llist_add(list, i2, sizeof(struct item));
		TEST_ASSERT_EQUAL(result, E_SUCCESS);

		ll_item = llist_find(list, i2);
		TEST_ASSERT_NOT_EQUAL(ll_item, 0);
		TEST_ASSERT_EQUAL(ll_item->data, i2);
		an_item = (struct item*)(ll_item->data);
		TEST_ASSERT_EQUAL(an_item->number, 2);
		TEST_ASSERT_EQUAL(an_item->character, '2');

		ll_item = llist_begin(list);
		TEST_ASSERT_NOT_EQUAL(ll_item, 0);
		TEST_ASSERT_EQUAL(ll_item->data, i1);
		an_item = (struct item*)(ll_item->data);
		TEST_ASSERT_EQUAL(an_item->number, 1);
		TEST_ASSERT_EQUAL(an_item->character, '1');
		
		result = llist_remove(list, i2);
		TEST_ASSERT_EQUAL(result, E_SUCCESS);

		ll_item = llist_find(list, i2);
		TEST_ASSERT_EQUAL(ll_item, 0);

		llist_destroy(list);
	}
}
