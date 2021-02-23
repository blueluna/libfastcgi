#include "fastcgi.h"
#include "errorcodes.h"
#include "request.h"
#include "parameter.h"
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

void fastcgi_version(int32_t *version_major, int32_t *version_minor
	, int32_t *version_patch)
{
	if (version_major != 0) {
		*version_major = 0;
	}
	if (version_minor != 0) {
		*version_minor = 1;
	}
	if (version_patch != 0) {
		*version_patch = 0;
	}
}

fastcgi_request_t* fastcgi_find_free_request(fastcgi_context_t *ctx)
{
	if (ctx == NULL || ctx->requests == NULL || ctx->requests->items == NULL) {
		return NULL;
	}
	fastcgi_request_t* request = NULL;
	fastcgi_request_t* item = NULL;
	llist_item_t *ptr = ctx->requests->items;
	while (ptr != NULL) {
		item = (fastcgi_request_t*)(ptr->data);
		if (item->id == 0) {
			request = (fastcgi_request_t*)(ptr->data);
			break;
		}
		ptr = ptr->next;
	}
	return request;
}

fastcgi_request_t* fastcgi_find_request(fastcgi_context_t *ctx, const uint16_t id)
{
	if (ctx == NULL || ctx->requests == NULL || ctx->requests->items == NULL) {
		return NULL;
	}
	fastcgi_request_t* request = NULL;
	fastcgi_request_t* item = NULL;
	llist_item_t *ptr = ctx->requests->items;
	while (ptr != NULL) {
		item = (fastcgi_request_t*)(ptr->data);
		if (item->id == id) {
			request = (fastcgi_request_t*)(ptr->data);
			break;
		}
		ptr = ptr->next;
	}
	return request;
}

fastcgi_request_t* fastcgi_take_request(fastcgi_context_t *ctx, const uint16_t id)
{
	if (ctx == NULL || ctx->requests == NULL || ctx->requests->items == NULL) {
		return NULL;
	}
	fastcgi_request_t* request = NULL;
	fastcgi_request_t* item = NULL;
	llist_item_t *ptr = ctx->requests->items;
	while (ptr != NULL) {
		item = (fastcgi_request_t*)(ptr->data);
		if (item->id == id) {
			llist_take(ctx->requests, ptr->data);
			request = (fastcgi_request_t*)(ptr->data);
			break;
		}
		ptr = ptr->next;
	}
	return request;
}

int32_t fastcgi_read_header(fcgi_record_header_t *header
	, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;
	if (len >= sizeof(fcgi_record_header_t)) {
		memcpy(header, data, sizeof(fcgi_record_header_t));
		header->request_id = ntohs(header->request_id);
		header->content_length = ntohs(header->content_length);
	}
	else {
		result = E_INVALID_SIZE;
	}
	return result;
}

int32_t fastcgi_begin_request(fastcgi_context_t *ctx
	, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;
	fcgi_record_begin_t record = {0};
	fastcgi_request_t *request = 0;

	if (len >= sizeof(fcgi_record_begin_t))
	{
		memcpy(&record, data, sizeof(fcgi_record_begin_t));
		record.role = ntohs(record.role);
		/* Only responder role is supported */
		if (record.role != FCGI_RESPONDER) {
			result = E_FCGI_INVALID_ROLE;
		}
	}
	else {
		result = E_INVALID_SIZE;
	}
	if (result == E_SUCCESS) {
		request = fastcgi_find_free_request(ctx);
		if (request == 0) {
			request = fastcgi_request_create();
			result = llist_add(ctx->requests, request
				, sizeof(fastcgi_request_t));
		}
		if (result == E_SUCCESS) {
			request->id = ctx->current_header->request_id;
			request->role = record.role;
			request->flags = record.flags;
			fastcgi_request_set_state(request, FASTCGI_RS_NEW);
		}
	}
	return result;
}

int32_t fastcgi_params(fastcgi_request_t *request
	, const char *data, const size_t len)
{
	int32_t n = 0;
	int32_t str_len[2];
	const char *name;
	const char *value;
	const char *ptr = data;
	int32_t left = 0;
	int32_t bytes_used = 0;
	int32_t bytes_delta = 0;
	int32_t bytes_delta_acc = 0;

	assert(len <= 0x7fffffff);
	left = (int32_t)len;

	if (len == 0) {
		fastcgi_request_set_state(request, FASTCGI_RS_PARAMS_DONE);
	}
	else {
		fastcgi_request_set_state(request, FASTCGI_RS_PARAMS);
	}

	while (left > 0) {
		/* Used to accumulate bytes used when decoding sizes */
		bytes_delta_acc = 0;
		/* Decode string size for name and value */
		for (n = 0; n < 2; n++) {
			bytes_delta = 0;
			/* Decode string size */
			if ((*ptr & 0x80) == 0x80) {
				if (left >= 4) {
					memcpy(&(str_len[n]), ptr, sizeof(uint32_t));
					str_len[n] = ntohl(str_len[n]) & 0x7fffffff;
					bytes_delta = 4;
				}
			}
			else {
				if (left >= 1) {
					str_len[n] = (*ptr) & 0x7F;
					bytes_delta = 1;
				}
			}
			if (bytes_delta == 0) {
				/* No bytes where decoded */
				break;
			}
			bytes_delta_acc += bytes_delta;
			left -= bytes_delta;
			ptr += bytes_delta;
		}
		if (bytes_delta > 0 && left >= (str_len[0] + str_len[1])) {
			name = ptr;
			ptr += str_len[0];
			left -= str_len[0];

			value = ptr;
			ptr += str_len[1];
			left -= str_len[1];

			fastcgi_request_parameter_add(request, name, str_len[0]
				, value, str_len[1]);

			bytes_used += (bytes_delta_acc + str_len[0] + str_len[1]);
		}
		else {
			break;
		}
	}
	return bytes_used;
}

int32_t fastcgi_stdin(fastcgi_request_t *request
	, const char *input, const size_t input_len)
{
	int32_t result = E_SUCCESS;

	if (input_len == 0) {
		fastcgi_request_set_state(request, FASTCGI_RS_STDIN_DONE);
	}
	else {
		fastcgi_request_set_state(request, FASTCGI_RS_STDIN);
	}

	result = buffer_write(request->content, input, input_len);

	return result;
}

int32_t fastcgi_process_input_buffer(fastcgi_context_t *ctx)
{
	int32_t result = E_SUCCESS;
	int32_t buffer_length = 0;
	const char *buffer_data = 0;
	int32_t bytes_used = 0;
	fastcgi_request_t *request = 0;

	request = fastcgi_find_request(ctx, ctx->current_header->request_id);
	if (request == 0) {
		if (ctx->current_header->type != FCGI_BEGIN_REQUEST) {
			result = E_REQUEST_NOT_FOUND;
		}
	}
	else {
		if (ctx->current_header->type == FCGI_BEGIN_REQUEST) {
			result = E_REQUEST_DUPLICATE;
		}
	}

	if (result == E_SUCCESS) {

		buffer_length = buffer_used(ctx->input);
		buffer_data = buffer_peek(ctx->input);
		switch (ctx->current_header->type) {
			case FCGI_BEGIN_REQUEST:
				result = fastcgi_begin_request(ctx, buffer_data, buffer_length);
				bytes_used = buffer_length;
				break;
			case FCGI_PARAMS:
				result = fastcgi_params(request, buffer_data, buffer_length);
				if (result >= 0) {
					bytes_used = result;
				}
				else {
					/* An error occured, clear buffer */
					bytes_used = buffer_length;
				}
				break;
			case FCGI_STDIN:
				result = fastcgi_stdin(request, buffer_data, buffer_length);
				if (result >= 0) {
					bytes_used = result;
				}
				else {
					/* An error occured, clear buffer */
					bytes_used = buffer_length;
				}
				break;
			case FCGI_DATA:
				/* Not supported, clear buffer */
			default:
				bytes_used = buffer_length;
				break;
		}
	}
	else {
		bytes_used = buffer_used(ctx->input);
	}
	buffer_read(ctx->input, 0, bytes_used);
	return result;
}

int32_t fastcgi_process_input(fastcgi_context_t *ctx
	, const char *data, const size_t len)
{
	int32_t length = 0;
	const char *ptr = 0;
	int32_t content_length = 0;
	int32_t padding_length = 0;
	int32_t bytes_left = 0;
	int32_t bytes_write = 0;
	int32_t bytes_used = 0;
	int32_t result = E_SUCCESS;

	assert(len <= 0x7fffffff);
	length = (int32_t)len;
	ptr = data;

	while (length > 0) {
		/* Try to read header if neccesary */
		if (ctx->read_state == 0) {
			result = fastcgi_read_header(ctx->current_header, ptr, length);
			if (result == E_SUCCESS) {
				length -= sizeof(fcgi_record_header_t);
				ptr += sizeof(fcgi_record_header_t);
				ctx->read_bytes = 0;
				ctx->read_state = 1;
				bytes_used += sizeof(fcgi_record_header_t);
			}
			else if (result == E_INVALID_SIZE && bytes_used > 0) {
				result = bytes_used;
				break;
			}
			else {
				break;
			}
		}
		content_length = ctx->current_header->content_length;
		padding_length = ctx->current_header->padding_length;
		bytes_write = 0;
		if (result == E_SUCCESS) {
			/* Try to write content data to buffer */
			bytes_left = content_length - ctx->read_bytes;
			if (bytes_left > 0) {
				bytes_write = bytes_left > length ? length : bytes_left;
				result = buffer_write(ctx->input, ptr, bytes_write);
				result = (result == bytes_write) ? E_SUCCESS : E_WRITE_FAILED;
			}
		}
		if (result == E_SUCCESS) {
			/* Update incoming data length ... whatever... */
			ptr += bytes_write;
			length -= bytes_write;
			bytes_used += bytes_write;
			ctx->read_bytes += bytes_write;

			/* Check if all content has been read */
			if (ctx->read_bytes >= content_length) {
				/* Check if all incoming data has been read */
				bytes_left = (content_length + padding_length) - ctx->read_bytes;
				if (bytes_left > 0) {
					/* Discard of padding */
					bytes_write = bytes_left > length ? length : bytes_left;
					ptr += bytes_write;
					length -= bytes_write;
					bytes_used += bytes_write;
					ctx->read_bytes += bytes_write;
					/* Update counter */
					bytes_left = (content_length + padding_length) - ctx->read_bytes;
				}
				/* Check if all incoming data has been read */
				if (bytes_left == 0) {
					result = fastcgi_process_input_buffer(ctx);
					ctx->read_state = 0;
				}
			}	
		}
	}
	if (result >= E_SUCCESS) {
		return bytes_used;
	}
	return result;
}

int32_t fastcgi_process_data(fastcgi_context_t *ctx
	, const char *data, const size_t len)
{
	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}
	if (data == 0) {
		return E_INVALID_ARGUMENT;
	}
	if (len == 0) {
		return E_SUCCESS;
	}
	return fastcgi_process_input(ctx, data, len);
}

/**** Public functions ******/

fastcgi_context_t * fastcgi_create()
{
	fastcgi_context_t *ctx = 0;

	ctx = malloc(sizeof(fastcgi_context_t));
	if (ctx != 0) {
		ctx->requests = llist_create(sizeof(fastcgi_request_t));
		if (ctx->requests == 0) {
			free(ctx);
			ctx = 0;
		}
		else {
			llist_register_dtor(ctx->requests
				, (llist_item_dtor_func)&fastcgi_request_destroy);
		}
	}
	if (ctx != 0) {
		ctx->input = buffer_create();
		if (ctx->input == 0) {
			llist_destroy(ctx->requests);
			ctx->requests = 0;
			free(ctx);
			ctx = 0;
		}
	}
	if (ctx != 0) {
		ctx->current_header = malloc(sizeof(fcgi_record_header_t));
		if (ctx->current_header == 0) {
			buffer_destroy(ctx->input);
			ctx->input = 0;
			llist_destroy(ctx->requests);
			ctx->requests = 0;
			free(ctx);
			ctx = 0;
		}
	}
	if (ctx != 0) {
		ctx->read_buffer_len = 1024;
		ctx->read_buffer = malloc(ctx->read_buffer_len);
		if (ctx->read_buffer == 0) {
			free(ctx->current_header);
			ctx->current_header = 0;
			buffer_destroy(ctx->input);
			ctx->input = 0;
			llist_destroy(ctx->requests);
			ctx->requests = 0;
			free(ctx);
			ctx = 0;
		}
	}
	if (ctx != 0) {
		ctx->read_state = 0;
		ctx->current_header->version = 0;
		ctx->current_header->type = 0;
		ctx->current_header->request_id = 0;
		ctx->current_header->content_length = 0;
		ctx->current_header->padding_length = 0;
	}
	return ctx;
}

void fastcgi_destroy(fastcgi_context_t *ctx)
{
	if (ctx != 0) {
		llist_destroy(ctx->requests);
		ctx->requests = 0;
		buffer_destroy(ctx->input);
		ctx->input = 0;
		free(ctx->current_header);
		ctx->current_header = 0;
		free(ctx->read_buffer);
		ctx->read_buffer = 0;
		ctx->read_buffer_len = 0;
		free(ctx);
	}
}

int32_t fastcgi_current_request_id(fastcgi_context_t *ctx)
{
	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}
	return ctx->current_header->request_id;
}

int32_t fastcgi_request_state(fastcgi_context_t *ctx, const uint16_t request_id)
{
	int32_t result = E_SUCCESS;
	fastcgi_request_t *request = 0;

	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}

	request = fastcgi_find_request(ctx, request_id);
	if (request == 0) {
		result = E_REQUEST_NOT_FOUND;
	}
	else {
		result = fastcgi_request_get_state(request, 0);
	}
	return result;
}

int32_t fastcgi_read(fastcgi_context_t *ctx
	, const char *input, const size_t input_len)
{
	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}
	return fastcgi_process_data(ctx, input, input_len);
}

int32_t fastcgi_write_output(fastcgi_context_t *ctx
	, const uint16_t request_id
	, const char *input, const size_t input_len)
{
	int32_t result = E_SUCCESS;
	fastcgi_request_t *request = 0;

	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}

	request = fastcgi_find_request(ctx, request_id);
	if (request == 0) {
		result = E_REQUEST_NOT_FOUND;
	}
	else {
		result = fastcgi_request_write_output(request, input, input_len);
	}
	
	return result;
}

int32_t fastcgi_write_error(fastcgi_context_t *ctx
	, const uint16_t request_id
	, const char *input, const size_t input_len)
{
	int32_t result = E_SUCCESS;
	fastcgi_request_t *request = 0;

	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}

	request = fastcgi_find_request(ctx, request_id);
	if (request == 0) {
		result = E_REQUEST_NOT_FOUND;
	}
	else {
		result = fastcgi_request_write_error(request, input, input_len);
	}
	
	return result;
}

int32_t fastcgi_finish(fastcgi_context_t *ctx, const uint16_t request_id)
{
	fastcgi_request_t *request = 0;
	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}
	request = fastcgi_find_request(ctx, request_id);
	if (request == 0) {
		return E_REQUEST_NOT_FOUND;
	}
	return fastcgi_request_finish(request, FCGI_REQUEST_COMPLETE, 0);
}

int32_t fastcgi_write(fastcgi_context_t *ctx
	, char *output, const size_t output_len, const uint16_t request_id)
{
	int32_t result = E_SUCCESS;
	int32_t state = 0;
	fastcgi_request_t *request = 0;

	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}

	request = fastcgi_find_request(ctx, request_id);

	if (request == 0) {
		result = E_REQUEST_NOT_FOUND;
	}
	else {
		result = fastcgi_request_output(request, output, output_len);
	}
	if (result >= 0) {
		state = fastcgi_request_get_state(request, 0);
		if ((state & FASTCGI_RS_FINISHED)) {
			/* Reset request */
			fastcgi_request_reset(request);
			request->id = 0;
			fastcgi_request_set_state(request, FASTCGI_RS_INIT);
		}
	}
	return result;
}
