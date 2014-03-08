/* FCGI Key/Value Pair
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef KLUNK_PARAM_H
#define KLUNK_PARAM_H

#include <stdlib.h>

typedef struct klunk_param_  {
	char		*name;
	char		*value;
} klunk_param_t;

/* Create a param "object" with the provided key/value pair */
klunk_param_t* klunk_param_create(const char *name, const size_t name_len
	, const char *value, const size_t value_len);

/* Destroy a param "object" */
void klunk_param_destroy(klunk_param_t *param);

#endif /* KLUNK_PARAM_H */
