#include "fcgi_param.h"
#include <string.h>

fcgi_param_t* fcgi_param_create(const char *name, const size_t name_len
	, const char *value, const size_t value_len)
{
	fcgi_param_t *param = 0;
	param = malloc(sizeof(fcgi_param_t));
	if (param != 0) {
		param->name = malloc(name_len + 1);
		param->value = malloc(value_len + 1);
		if (param->name == 0 || param->value == 0) {
			if (param->name != 0) {
				free(param->name);
			}
			if (param->value != 0) {
				free(param->value);
			}
			free(param);
			param = 0;
		}
		else {
			memcpy(param->name, name, name_len);
			param->name[name_len] = 0;
			memcpy(param->value, value, value_len);
			param->value[value_len] = 0;
		}
	}
	return param;
}

void fcgi_param_destroy(fcgi_param_t *param)
{
	if (param != 0) {
		free(param->name);
		param->name = 0;
		free(param->value);
		param->value = 0;
		free(param);
	}
}
