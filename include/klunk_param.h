/* FCGI Key/Value Pair
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef KLUNK_PARAM_H
#define KLUNK_PARAM_H

#include <stdlib.h>
#include <stdint.h>

typedef struct klunk_param_  {
	size_t		name_allocated;
	size_t		value_allocated;
	size_t		name_len;
	size_t		value_len;
	char		*name;
	char		*value;
} klunk_param_t;

/* Create a param "object" with the provided key/value pair */
klunk_param_t* klunk_param_create(const char *name, const size_t name_len
	, const char *value, const size_t value_len);

/* Destroy a param "object" */
void klunk_param_destroy(klunk_param_t *param);

/* Set the key/value pair */
int32_t klunk_param_set(klunk_param_t* param
	, const char *name, const size_t name_len
	, const char *value, const size_t value_len);

/* Clear the key/value pair */
int32_t klunk_param_reset(klunk_param_t* param);

int32_t klunk_param_is_free(klunk_param_t *param);

#endif /* KLUNK_PARAM_H */
