#include <stdlib.h>
#include <arpa/inet.h>

#include "errorcodes.h"
#include "request.h"
#include "utilities.h"
#include "parameter.h"
#include "protocol.h"

int32_t fastcgi_request_param_reset(void *data, void *user_data)
{
	(void)user_data;
	fastcgi_parameter_reset((fastcgi_parameter_t*)data);
	return 1;
}

int32_t fastcgi_request_param_is_free(void *data, void *user_data)
{
	(void)user_data;
	int32_t result = fastcgi_parameter_is_free((fastcgi_parameter_t*)data);
	if (result == 0) {
		return 0;
	}
	return 1;
}

void fastcgi_request_param_destroy(void *data)
{
	fastcgi_parameter_destroy((fastcgi_parameter_t*)data);
}

fastcgi_request_t* fastcgi_request_create()
{
	fastcgi_request_t *request = 0;
	request = malloc(sizeof(fastcgi_request_t));
	if (request != 0) {
		request->id = 0;
		request->role = 0;
		request->flags = 0;
		request->state = 0;
		request->params = 0;
		request->content = 0;
		request->output = 0;

		request->params = llist_create(sizeof(fastcgi_parameter_t));
		if (request->params == 0) {
			fastcgi_request_destroy(request);
			request = 0;
		}
		else {
			llist_register_dtor(request->params
				, fastcgi_request_param_destroy);
		}
	}
	if (request != 0) {
		request->content = buffer_create();
		if (request->content == 0) {
			fastcgi_request_destroy(request);
			request = 0;
		}
	}
	if (request != 0) {
		request->output = buffer_create();
		if (request->output == 0) {
			fastcgi_request_destroy(request);
			request = 0;
		}
	}
	if (request != 0) {
		request->error = buffer_create();
		if (request->error == 0) {
			fastcgi_request_destroy(request);
			request = 0;
		}
	}
	return request;
}

void fastcgi_request_destroy(fastcgi_request_t *request)
{
	if (request != 0) {
		buffer_destroy(request->error);
		request->error = 0;
		buffer_destroy(request->output);
		request->output = 0;
		buffer_destroy(request->content);
		request->content = 0;
		llist_destroy(request->params);
		request->params = 0;
		free(request);
	}
}

void fastcgi_request_reset(fastcgi_request_t *request)
{
	if (request != 0) {
		buffer_clear(request->error);
		buffer_clear(request->output);
		buffer_clear(request->content);
		llist_foreach(request->params, fastcgi_request_param_reset, NULL);
	}
}

int32_t fastcgi_request_get_state(fastcgi_request_t *request, const uint16_t mask)
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

int32_t fastcgi_request_set_state(fastcgi_request_t *request, const uint16_t state)
{
	int32_t result = E_SUCCESS;
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	switch (state) {
		case FASTCGI_RS_INIT:
			/* Clear state to init */
			request->state = FASTCGI_RS_INIT;
			break;
		case FASTCGI_RS_NEW:
			request->state |= FASTCGI_RS_NEW;
			break;
		case FASTCGI_RS_PARAMS:
			request->state |= FASTCGI_RS_PARAMS;
			break;
		case FASTCGI_RS_PARAMS_DONE:
			request->state |= FASTCGI_RS_PARAMS_DONE;
			break;
		case FASTCGI_RS_STDIN:
			request->state |= FASTCGI_RS_STDIN;
			break;
		case FASTCGI_RS_STDIN_DONE:
			request->state |= FASTCGI_RS_STDIN_DONE;
			break;
		case FASTCGI_RS_STDOUT:
			request->state |= FASTCGI_RS_STDOUT;
			break;
		case FASTCGI_RS_STDOUT_DONE:
			request->state |= FASTCGI_RS_STDOUT_DONE;
			break;
		case FASTCGI_RS_STDERR:
			request->state |= FASTCGI_RS_STDERR;
			break;
		case FASTCGI_RS_STDERR_DONE:
			request->state |= FASTCGI_RS_STDERR_DONE;
			break;
		case FASTCGI_RS_FINISH:
			request->state |= FASTCGI_RS_FINISH;
			break;
		case FASTCGI_RS_FINISHED:
			request->state |= FASTCGI_RS_FINISHED;
			break;
		default:
			result = E_INVALID_ARGUMENT;
	}
	if (result == E_SUCCESS) {
		result = (int32_t)request->state;
	}
	return result;
}

int32_t fastcgi_request_parameter_add(fastcgi_request_t *request
	, const char *name, const size_t name_len
	, const char *value, const size_t value_len)
{
	int32_t result = E_SUCCESS;
	llist_item_t *item = 0;
	fastcgi_parameter_t *param = 0;

	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	item = llist_find_item_match(request->params
		, fastcgi_request_param_is_free, NULL);
	if (item != 0) {
		param = (fastcgi_parameter_t*)(item->data);
		result = fastcgi_parameter_set(param, name, name_len, value, value_len);
	}
	else {
		param = fastcgi_parameter_create(name, name_len, value, value_len);
		if (param != 0) {
			result = llist_add(request->params, param, sizeof(fastcgi_parameter_t));
		}
	}
	return result;
}

int32_t fastcgi_prepare_header(fastcgi_request_t *request
	, fcgi_record_header_t *header)
{
	int32_t result = E_SUCCESS;
	if (header == 0) {
		return E_INVALID_OBJECT;
	}
	else {
		header->version = FCGI_VERSION_1;
		header->type = FCGI_UNKNOWN_TYPE;
		header->request_id = htons(request->id);
		header->content_length = 0;
		header->padding_length = 0;
		header->reserved = 0;
	}
	return result;
}

int32_t fastcgi_request_calculate_size(const int32_t input_len)
{
	return size8b(input_len) + sizeof(fcgi_record_header_t);
}

int32_t fastcgi_request_generate_record(fastcgi_request_t *request
	, char *output, const int32_t output_len
	, const uint8_t type
	, const char *input, const int32_t input_len)
{
	int32_t result = E_SUCCESS;
	fcgi_record_header_t header = {0};
	int32_t total_len;
	int32_t offset = 0;
	uint16_t padding_len;
	const char *padding = "\0\0\0\0\0\0\0\0";
	char *ptr = output;

	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	if (output == 0) {
		return E_INVALID_ARGUMENT;
	}
	if ((request->state & FASTCGI_RS_FINISHED)) {
		return E_INVALID_ARGUMENT;
	}

	total_len = fastcgi_request_calculate_size(input_len);

	if (output_len < total_len) {
		result = E_INVALID_SIZE;
	}
	if (result == E_SUCCESS) {
		result = fastcgi_prepare_header(request, &header);
	}
	if (result == E_SUCCESS) {
		/* Prepare header for user content */
		header.type = type;
		header.content_length = htons(input_len);
		padding_len = size8b(input_len) - input_len;
		header.padding_length = (uint8_t)(padding_len);
		/* Write header */
		memcpy(ptr, &header, sizeof(fcgi_record_header_t));
		offset += sizeof(fcgi_record_header_t);
		memcpy(ptr+offset, input, input_len);
		offset += input_len;
		memcpy(ptr+offset, padding, padding_len);
		offset += padding_len;
		if (input_len == 0) {
			if (type == FCGI_STDOUT) {
				fastcgi_request_set_state(request, FASTCGI_RS_STDOUT);
				fastcgi_request_set_state(request, FASTCGI_RS_STDOUT_DONE);
			}
			else if (type == FCGI_STDERR) {
				fastcgi_request_set_state(request, FASTCGI_RS_STDERR);
				fastcgi_request_set_state(request, FASTCGI_RS_STDERR_DONE);
			}
		}
		else {
			if (type == FCGI_STDOUT) {
				fastcgi_request_set_state(request, FASTCGI_RS_STDOUT);
			}
			else if (type == FCGI_STDERR) {
				fastcgi_request_set_state(request, FASTCGI_RS_STDERR);
			}
			else if (type == FCGI_END_REQUEST) {
				fastcgi_request_set_state(request, FASTCGI_RS_FINISH);
			}
		}
		result = offset;
	}
	return result;
}

int32_t fastcgi_request_store(fastcgi_request_t *request
	, const uint8_t type
	, const char *input, const size_t input_len)
{
	buffer_t *buf = 0;

	if (type == FCGI_STDOUT) {
		buf = request->output;
	}
	else if (type == FCGI_STDERR) {
		buf = request->error;
	}
	else {
		return E_INVALID_ARGUMENT;
	}

	return buffer_write(buf, input, input_len);
}

int32_t fastcgi_request_write_output(fastcgi_request_t *request
	, const char *input, const size_t input_len)
{
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	return fastcgi_request_store(request, FCGI_STDOUT, input, input_len);
}

int32_t fastcgi_request_write_error(fastcgi_request_t *request
	, const char *input, const size_t input_len)
{
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	return fastcgi_request_store(request, FCGI_STDERR, input, input_len);	
}

int32_t fastcgi_request_finish(fastcgi_request_t *request
	, const uint32_t app_status, const uint8_t protocol_status)
{
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	request->protocol_status = protocol_status;
	request->app_status = app_status;
	fastcgi_request_set_state(request, FASTCGI_RS_FINISH);
	return E_SUCCESS;
}

int32_t fastcgi_request_output(fastcgi_request_t *request
	, char *output, const size_t output_len)
{
	int32_t result = E_SUCCESS;
	size_t stored_len = 0;
	size_t content_len = 0;
	size_t use_len = 0;
	int32_t finish = 0;

	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	if (output == 0) {
		return E_INVALID_ARGUMENT;
	}
	if ((request->state & FASTCGI_RS_FINISHED)) {
		return E_SUCCESS;
	}
	if (output_len < sizeof(fcgi_record_header_t)) {
		return E_INVALID_SIZE;
	}

	finish = (request->state & FASTCGI_RS_FINISH) > 0;
	content_len = output_len - sizeof(fcgi_record_header_t);

	stored_len = buffer_used(request->error);
	if (stored_len > 0) {
		if (content_len > 0) {
			use_len = stored_len > content_len ? content_len : stored_len;
			result = fastcgi_request_generate_record(request, output, output_len
				, FCGI_STDERR, buffer_peek(request->error), use_len);
			if (result > 0) {
				buffer_read(request->error, 0, use_len);
			}
		}
		else {
			result = E_INVALID_SIZE;
		}
		return result;
	}
	else if (finish && (request->state & FASTCGI_RS_STDERR)
		&& ((request->state & FASTCGI_RS_STDERR_DONE) == 0)) {
		return fastcgi_request_generate_record(request, output, output_len
			, FCGI_STDERR, 0, 0);
	}

	stored_len = buffer_used(request->output);
	if (stored_len > 0) {
		if (content_len > 0) {
			use_len = stored_len > content_len ? content_len : stored_len;
			result = fastcgi_request_generate_record(request, output, output_len
				, FCGI_STDOUT, buffer_peek(request->output), use_len);
			if (result > 0) {
				buffer_read(request->output, 0, use_len);
			}
		}
		else {
			result = E_INVALID_SIZE;
		}
		return result;
	}
	else if (finish && (request->state & FASTCGI_RS_STDOUT)
		&& ((request->state & FASTCGI_RS_STDOUT_DONE) == 0)) {
		return fastcgi_request_generate_record(request, output, output_len
			, FCGI_STDOUT, 0, 0);
	}

	if (finish) {
		if (content_len >= 8) {
			fcgi_record_end_t record = {
				.app_status = request->app_status,
				.protocol_status = request->protocol_status,
				.reserved = {0}
			};
			result = fastcgi_request_generate_record(request, output, output_len
				, FCGI_END_REQUEST
				, (const char*)&record, (uint16_t)sizeof(fcgi_record_end_t));
			if (result >= 0) {
				fastcgi_request_set_state(request, FASTCGI_RS_FINISHED);
			}
		}
		else {
			result = E_INVALID_SIZE;
		}
	}

	return result;
}
