/* FCGI Key/Value Pair
 * (C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef FASTCGI_PARAMETER_H
#define FASTCGI_PARAMETER_H

#include <stdlib.h>
#include <stdint.h>

typedef struct fastcgi_param_  {
	size_t		name_allocated;
	size_t		value_allocated;
	size_t		name_len;
	size_t		value_len;
	char		*name;
	char		*value;
} fastcgi_parameter_t;

/* Create a param "object" with the provided key/value pair */
fastcgi_parameter_t* fastcgi_parameter_create(const char *name, const size_t name_len
	, const char *value, const size_t value_len);

/* Destroy a param "object" */
void fastcgi_parameter_destroy(fastcgi_parameter_t *param);

/* Set the key/value pair */
int32_t fastcgi_parameter_set(fastcgi_parameter_t* param
	, const char *name, const size_t name_len
	, const char *value, const size_t value_len);

/* Clear the key/value pair */
int32_t fastcgi_parameter_reset(fastcgi_parameter_t* param);

int32_t fastcgi_parameter_is_free(fastcgi_parameter_t *param);

#endif /* FASTCGI_PARAMETER_H */
