/* A single linked list
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef ES_LLIST_H
#define ES_LLIST_H

#include <stdint.h>
#include <string.h>

typedef void (*llist_item_dtor_func)(void*);
typedef int32_t (*llist_iterator_func)(void* data, void *user_data);

typedef struct llist_item_ {
	struct llist_item_	*next;
	void				*data;
} llist_item_t;

typedef struct llist_ {
	llist_item_t			*items;
	size_t					item_size;
	llist_item_dtor_func	item_dtor;
} llist_t;

/* Create a linked list */
llist_t*		llist_create(const size_t item_size);

/* Destroy the linked list */
void 			llist_destroy(llist_t *list);

/* Register user supplied destructor, set item_dtor to zero (0) to use free */
int32_t			llist_register_dtor(llist_t *list, llist_item_dtor_func item_dtor);

/* Add a new item to the linked list */
int32_t			llist_add(llist_t *list, void *data, const size_t item_size);

/* Take a item from the linked list without destroying it */
int32_t			llist_take(llist_t *list, const void *data);

/* Remove a item from the linked list */
int32_t			llist_remove(llist_t *list, const void *data);

/* Find an item in the linked list */
llist_item_t*	llist_find_item(llist_t *list, const void *data);

/* First item in the linked list */
llist_item_t*	llist_begin(llist_t *list);

/* Find an item in the linked list using a user supplied match function and
 * key. The loop stops when the match function returns true.
 */
llist_item_t*	llist_find_item_match(llist_t *list
	, llist_iterator_func match_func, void *user_data);


/* Iterate through the list and run the provided function. The loop stops when
 * the provided function returns false.
 */
void llist_foreach(llist_t *list, llist_iterator_func func
	, void *user_data);

#endif /* ES_LLIST_H */
