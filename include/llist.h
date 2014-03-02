/* A single linked list
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT licence.
 */

#ifndef ES_LLIST_H
#define ES_LLIST_H

#include <stdint.h>
#include <string.h>

typedef void (*llist_item_dtor)(void*);

typedef struct llist_item_ {
	struct llist_item_	*next;
	void				*data;
} llist_item_t;

typedef struct llist_ {
	llist_item_t	*items;
	size_t			item_size;
	llist_item_dtor	item_dtor;
} llist_t;

/* Create a linked list */
llist_t*		llist_create(const size_t item_size);

/* Destroy the linked list */
void 			llist_destroy(llist_t *list);

/* Register user supplied destructor, set item_dtor to zero (0) to use free */
int32_t			llist_register_dtor(llist_t *list, llist_item_dtor item_dtor);

/* Add a new item to the linked list */
int32_t			llist_add(llist_t *list, void *data, const size_t item_size);

/* Remove a item from the linked list */
int32_t			llist_remove(llist_t *list, const void *data);

/* Remove a item from the linked list */
int32_t			llist_remove_ex(llist_t *list, const void *data, llist_item_dtor item_dtor);

/* Find an item in the linked list */
llist_item_t*	llist_find(llist_t *list, const void *data);

/* First item in the linked list */
llist_item_t*	llist_begin(llist_t *list);

#endif /* ES_LLIST_H */
