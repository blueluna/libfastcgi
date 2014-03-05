#include "errorcodes.h"
#include "klunk_request.h"
#include "fcgi_param.h"

#include <stdlib.h>

klunk_request_t* klunk_request_create()
{
	klunk_request_t *request = 0;
	request = malloc(sizeof(klunk_request_t));
	if (request != 0) {
		request->id = 0;
		request->role = 0;
		request->flags = 0;
		request->state = 0;
		request->params = 0;
		request->content = 0;

		request->params = llist_create(sizeof(fcgi_param_t));
		if (request->params == 0) {
			free(request);
			request = 0;
		}
		else {
			llist_register_dtor(request->params
				, (llist_item_dtor)&fcgi_param_destroy);
		}
	}
	if (request != 0) {
		request->content = buffer_create();
		if (request->content == 0) {
			llist_destroy(request->params);
			request->params = 0;
			free(request);
			request = 0;
		}
	}
	return request;
}

void klunk_request_destroy(klunk_request_t *request)
{
	if (request != 0) {
		buffer_destroy(request->content);
		request->content = 0;
		llist_destroy(request->params);
		request->params = 0;
		free(request);
	}
}

int32_t klunk_request_get_state(klunk_request_t *request, const uint16_t mask)
{
	int32_t result = E_SUCCESS;
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	if (mask == 0) {
		result = (int32_t)(request->state);
	}
	else {
		result = (int32_t)(request->state & mask);
	}
	return result;
}

int32_t klunk_request_set_state(klunk_request_t *request, const uint16_t state)
{
	int32_t result = E_SUCCESS;
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	switch (state) {
		case KLUNK_RS_INIT:
			/* Clear state to init */
			request->state = KLUNK_RS_INIT;
			break;
		case KLUNK_RS_NEW:
			request->state |= KLUNK_RS_NEW;
			break;
		case KLUNK_RS_PARAMS:
			request->state |= KLUNK_RS_PARAMS;
			break;
		case KLUNK_RS_PARAMS_DONE:
			request->state |= KLUNK_RS_PARAMS_DONE;
			break;
		case KLUNK_RS_STDIN:
			request->state |= KLUNK_RS_STDIN;
			break;
		case KLUNK_RS_STDIN_DONE:
			request->state |= KLUNK_RS_STDIN_DONE;
			break;
		case KLUNK_RS_STDOUT:
			request->state |= KLUNK_RS_STDOUT;
			break;
		case KLUNK_RS_STDOUT_DONE:
			request->state |= KLUNK_RS_STDOUT_DONE;
			break;
		case KLUNK_RS_STDERR:
			request->state |= KLUNK_RS_STDERR;
			break;
		case KLUNK_RS_STDERR_DONE:
			request->state |= KLUNK_RS_STDERR_DONE;
			break;
		case KLUNK_RS_FINISHED:
			request->state |= KLUNK_RS_FINISHED;
			break;
		default:
			result = E_INVALID_ARGUMENT;
	}
	if (result == E_SUCCESS) {
		result = (int32_t)request->state;
	}
	return result;
}
