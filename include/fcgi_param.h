
#ifndef FCGI_PARAM_H
#define FCGI_PARAM_H

#include <stdlib.h>

typedef struct fcgi_param_  {
	char		*name;
	char		*value;
} fcgi_param_t;

fcgi_param_t* fcgi_param_create(const char *name, const size_t name_len
	, const char *value, const size_t value_len);

void fcgi_param_destroy(fcgi_param_t *param);

#endif /* FCGI_PARAM_H */
