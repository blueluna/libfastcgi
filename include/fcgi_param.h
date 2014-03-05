/* FCGI Key/Value Pair
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef FCGI_PARAM_H
#define FCGI_PARAM_H

#include <stdlib.h>

typedef struct fcgi_param_  {
	char		*name;
	char		*value;
} fcgi_param_t;

/* Create a param "object" with the provided key/value pair */
fcgi_param_t* fcgi_param_create(const char *name, const size_t name_len
	, const char *value, const size_t value_len);

/* Destroy a param "object" */
void fcgi_param_destroy(fcgi_param_t *param);

#endif /* FCGI_PARAM_H */
