#include <stdlib.h>
#include <arpa/inet.h>

#include <stdio.h>

#include "errorcodes.h"
#include "klunk_request.h"
#include "klunk_utils.h"
#include "fcgi_param.h"
#include "fcgi_protocol.h"


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
		request->output = 0;

		request->params = llist_create(sizeof(fcgi_param_t));
		if (request->params == 0) {
			klunk_request_destroy(request);
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
			klunk_request_destroy(request);
			request = 0;
		}
	}
	if (request != 0) {
		request->output = buffer_create();
		if (request->output == 0) {
			klunk_request_destroy(request);
			request = 0;
		}
	}
	if (request != 0) {
		request->error = buffer_create();
		if (request->error == 0) {
			klunk_request_destroy(request);
			request = 0;
		}
	}
	return request;
}

void klunk_request_destroy(klunk_request_t *request)
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

int32_t klunk_prepare_header(klunk_request_t *request
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

int32_t klunk_request_calculate_size(const int32_t input_len)
{
	return klunk_size8b(input_len) + sizeof(fcgi_record_header_t);
}

int32_t klunk_request_generate_record(klunk_request_t *request
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
	if ((request->state & KLUNK_RS_WRITE_FINISHED)) {
		return E_INVALID_ARGUMENT;
	}
	if ((request->state & KLUNK_RS_FINISHED)) {
		if (type == FCGI_STDOUT || type == FCGI_STDERR) {
			return E_INVALID_ARGUMENT;
		}
	}
	if (type == FCGI_STDOUT && (request->state & KLUNK_RS_STDOUT_DONE)) {
		return E_INVALID_ARGUMENT;
	}
	if (type == FCGI_STDERR && (request->state & KLUNK_RS_STDERR_DONE)) {
		return E_INVALID_ARGUMENT;
	}

	total_len = klunk_request_calculate_size(input_len);

	if (output_len < total_len) {
		result = E_INVALID_SIZE;
	}
	if (result == E_SUCCESS) {
		result = klunk_prepare_header(request, &header);
	}
	if (result == E_SUCCESS) {
		/* Prepare header for user content */
		header.type = type;
		header.content_length = htons(input_len);
		padding_len = klunk_size8b(input_len) - input_len;
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
				klunk_request_set_state(request, KLUNK_RS_STDOUT);
				klunk_request_set_state(request, KLUNK_RS_STDOUT_DONE);
			}
			else if (type == FCGI_STDERR) {
				klunk_request_set_state(request, KLUNK_RS_STDERR);
				klunk_request_set_state(request, KLUNK_RS_STDERR_DONE);
			}
		}
		else {
			if (type == FCGI_STDOUT) {
				klunk_request_set_state(request, KLUNK_RS_STDOUT);
			}
			else if (type == FCGI_STDERR) {
				klunk_request_set_state(request, KLUNK_RS_STDERR);
			}
			else if (type == FCGI_END_REQUEST) {
				klunk_request_set_state(request, KLUNK_RS_FINISHED);
			}
		}
		result = offset;
	}
	return result;
}

int32_t klunk_request_write_record(klunk_request_t *request
	, const uint8_t type
	, const char *input, const size_t input_len)
{
	int32_t result = E_SUCCESS;
	int32_t total_len;
	buffer_t *output = 0;

	total_len = klunk_request_calculate_size(input_len);
	if (type == FCGI_STDOUT) {
		output = request->output;
	}
	else if (type == FCGI_STDERR) {
		output = request->error;
	}

	if ((int32_t)buffer_free(output) < total_len) {
		result = buffer_reserve(output, buffer_used(output) + total_len);
	}
	if (result >= 0) {
		result = klunk_request_generate_record(request
			, buffer_peek(output), buffer_free(output)
			, type
			, input, input_len
			);
	}

	return result;
}

int32_t klunk_request_write_output(klunk_request_t *request
	, const char *input, const size_t input_len)
{
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	return klunk_request_write_record(request, FCGI_STDOUT, input, input_len);
}

int32_t klunk_request_write_error(klunk_request_t *request
	, const char *input, const size_t input_len)
{
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	return klunk_request_write_record(request, FCGI_STDERR, input, input_len);	
}

int32_t klunk_request_read(klunk_request_t *request
	, char *output, const size_t output_len)
{
	int32_t result = E_SUCCESS;

	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	if (output == 0) {
		return E_INVALID_ARGUMENT;
	}
	
	if ((request->state & KLUNK_RS_STDOUT) && (request->state & KLUNK_RS_STDOUT_DONE)) {
		result = buffer_read(request->output, output, output_len);
	}
	else if ((request->state & KLUNK_RS_STDERR) && (request->state & KLUNK_RS_STDERR_DONE)) {
		result = buffer_read(request->error, output, output_len);
	}
	else if ((request->state & KLUNK_RS_FINISHED)) {
		fcgi_record_end_t record = {
			.app_status = request->app_status,
			.protocol_status = request->protocol_status,
			.reserved = {0}
		};
		result = klunk_request_generate_record(request, output, output_len
			, FCGI_END_REQUEST
			, (const char*)&record, (uint16_t)sizeof(fcgi_record_end_t));
		if (result >= 0) {
			return klunk_request_set_state(request, KLUNK_RS_WRITE_FINISHED);
		}
	}

	return result;
}

int32_t klunk_request_finish(klunk_request_t *request
	, const uint32_t app_status, const uint8_t protocol_status)
{
	if (request == 0) {
		return E_INVALID_OBJECT;
	}
	request->protocol_status = protocol_status;
	request->app_status = app_status;
	return klunk_request_set_state(request, KLUNK_RS_FINISHED);
}
