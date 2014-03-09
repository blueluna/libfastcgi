/* A single linked list
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT licence.
 */

#include "llist.h"
#include "errorcodes.h"
#include <stdlib.h>

/* Helper function for destroying list item data */
void llist_destroy_item_data(llist_t *list, llist_item_t *item)
{
	/* Is there data? */
	if (item->data != 0) {
		/* Do we have a user supplied destructor? */
		if (list->item_dtor == 0) {
			free(item->data);
		}
		else {
			(*(list->item_dtor))(item->data);
		}
		item->data = 0;
	}
}

llist_t* llist_create(const size_t item_size_)
{
	llist_t *list = malloc(sizeof(llist_t));
	if (list != 0) {
		list->item_size = item_size_;
		list->item_dtor = 0;
		list->items = 0;
		return list;
	}
	return 0;
}

void llist_destroy(llist_t *list)
{
	if (list == 0) {
		return;
	}
	llist_item_t *ptr = list->items;
	llist_item_t *next = 0;
	while (ptr != 0) {
		next = ptr->next;
		llist_destroy_item_data(list, ptr);
		ptr->next = 0;
		free(ptr);
		ptr = next;
	}
	free(list);
}

int32_t llist_register_dtor(llist_t *list, llist_item_dtor_func item_dtor)
{
	if (list == 0) {
		return E_INVALID_ARGUMENT;
	}
	list->item_dtor = item_dtor;
	return E_SUCCESS;
}

int32_t llist_add(llist_t *list, void *data, const size_t item_size)
{
	if (list == 0) {
		return E_INVALID_ARGUMENT;
	}
	if (item_size > 0 && data == 0) {
		return E_INVALID_ARGUMENT;
	}
	if (item_size != list->item_size) {
		return E_INVALID_SIZE;
	}

	llist_item_t *item = malloc(sizeof(llist_item_t));
	if (item == 0) {
		return E_MEMORY_ALLOCATION_FAILED;
	}
	item->data = data;
	item->next = 0;

	int32_t result = E_SUCCESS;
	llist_item_t *ptr = list->items;
	llist_item_t *prev = 0;
	while (ptr != 0) {
		/* check if there is an item with the same id */
		if (ptr->data == data) {
			result = E_DUPLICATE;
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	if (result == E_SUCCESS) {
		/* prev == 0, insert into empty list */
		if (prev == 0) {
			list->items = item;
		}
		else {
			prev->next = item;
		}
	}
	else {
		free(item);
	}
	return result;
}

int32_t llist_take(llist_t *list, const void *data)
{
	if (list == 0) {
		return E_INVALID_ARGUMENT;
	}
	int32_t result = E_NOT_FOUND;
	llist_item_t *ptr = list->items;
	llist_item_t *prev = 0;
	while (ptr != 0) {
		if (ptr->data == data) {
			if (prev == 0) {
				/* Removing first item */
				list->items = ptr->next;
			}
			else {
				prev->next = ptr->next;
			}
			free(ptr);
			result = E_SUCCESS;
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	return result;
}

int32_t llist_remove(llist_t *list, const void *data)
{
	if (list == 0) {
		return E_INVALID_ARGUMENT;
	}
	int32_t result = E_NOT_FOUND;
	llist_item_t *ptr = list->items;
	llist_item_t *prev = 0;
	while (ptr != 0) {
		if (ptr->data == data) {
			if (prev == 0) {
				/* Removing first item */
				list->items = ptr->next;
			}
			else {
				prev->next = ptr->next;
			}
			llist_destroy_item_data(list, ptr);
			free(ptr);
			result = E_SUCCESS;
			break;
		}
		prev = ptr;
		ptr = ptr->next;
	}
	return result;
}

llist_item_t* llist_find_item(llist_t *list, const void *data)
{
	if (list == 0 || data == 0) {
		return 0;
	}
	llist_item_t *ptr = list->items;
	while (ptr != 0) {
		if (ptr->data == data) {
			break;
		}
		ptr = ptr->next;
	}
	return ptr;
}

llist_item_t* llist_begin(llist_t *list)
{
	if (list == 0) {
		return 0;
	}
	return list->items;
}

llist_item_t*	llist_find_item_match(llist_t *list
	, llist_iterator_func func, const void *key)
{
	if (list == 0 || func == 0) {
		return 0;
	}
	llist_item_t *ptr = list->items;
	while (ptr != 0) {
		if ((*(func))(ptr->data, key)) {
			break;
		}
		ptr = ptr->next;
	}
	return ptr;
}

/* Iterate through the list and run the provided function
 */
void llist_foreach(llist_t *list, llist_iterator_func func
	, const void *user_data)
{
	if (list == 0 || func == 0) {
		return;
	}
	llist_item_t *ptr = list->items;
	while (ptr != 0) {
		if (!(*(func))(ptr->data, user_data)) {
			break;
		}
		ptr = ptr->next;
	}
	return;
}
