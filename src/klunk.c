#include "klunk.h"
#include "errorcodes.h"
#include "klunk_request.h"
#include "fcgi_param.h"
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

uint16_t klunk_size8b(const uint16_t size)
{
	uint16_t s;
	if ((size % 8) == 0) {
		return size;
	}
	else {
		s = ((size / 8) + 1) * 8;
		return s;
	}
}

klunk_request_t* klunk_find_request(klunk_context_t *ctx, const uint16_t id)
{
	if (ctx == 0 || ctx->sessions == 0 || ctx->sessions->items == 0) {
		return 0;
	}
	klunk_request_t* session = 0;
	klunk_request_t* item = 0;
	llist_item_t *ptr = ctx->sessions->items;
	while (ptr != 0) {
		item = (klunk_request_t*)(ptr->data);
		if (item->id == id) {
			session = (klunk_request_t*)(ptr->data);
			break;
		}
		ptr = ptr->next;
	}
	return session;
}

int32_t klunk_read_header(fcgi_record_header_t *header, const char *data
	, const size_t len)
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

int32_t klunk_begin_request(klunk_context_t *ctx, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;
	fcgi_record_begin_t record = {0};
	klunk_request_t *session = 0;

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
		session = klunk_request_create();
		session->id = ctx->current_header->request_id;
		session->role = record.role;
		session->flags = record.flags;
		session->state = FCGI_BEGIN_REQUEST;
		result = llist_add(ctx->sessions, session, sizeof(klunk_request_t));
	}
	return result;
}

int32_t klunk_params(klunk_request_t *session, const char *data, const size_t len)
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

	session->state = FCGI_PARAMS;

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

			fcgi_param_t *param = fcgi_param_create(name, str_len[0]
				, value, str_len[1]);
			llist_add(session->params, param, sizeof(fcgi_param_t));

			bytes_used += (bytes_delta_acc + str_len[0] + str_len[1]);
		}
		else {
			break;
		}
	}
	return bytes_used;
}

int32_t klunk_stdin(klunk_request_t *session, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;

	session->state = FCGI_STDIN;
	result = buffer_write(session->content, data, len);

	return result;
}

int32_t klunk_process_input_buffer(klunk_context_t *ctx)
{
	int32_t result = E_SUCCESS;
	int32_t buffer_length = 0;
	const char *buffer_data = 0;
	int32_t bytes_used = 0;
	klunk_request_t *session = 0;

	session = klunk_find_request(ctx, ctx->current_header->request_id);
	if (session == 0) {
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
		bytes_used = 0;
		switch (ctx->current_header->type) {
			case FCGI_BEGIN_REQUEST:
				result = klunk_begin_request(ctx, buffer_data, buffer_length);
				if (result >= 0) {
					result = KLUNK_NEW_REQUEST;
				}
				bytes_used = buffer_length;
				break;
			case FCGI_PARAMS:
				result = klunk_params(session, buffer_data, buffer_length);
				if (result >= 0) {
					bytes_used = result;
					if (ctx->current_header->content_length == 0) {
						/* We have received all PARAMS data */
						result = KLUNK_PARAMS_DONE;
					}
					else {
						result = E_SUCCESS;
					}
				}
				else {
					/* An error occured, clear buffer */
					bytes_used = buffer_length;
				}
				break;
			case FCGI_STDIN:
				result = klunk_stdin(session, buffer_data, buffer_length);
				if (result >= 0) {
					bytes_used = result;
					if (ctx->current_header->content_length == 0) {
						/* We have received all STDIN data */
						result = KLUNK_STDIN_DONE;
					}
					else {
						result = E_SUCCESS;
					}
				}
				else {
					/* An error occured, clear buffer */
					bytes_used = buffer_length;
				}
				break;
			case FCGI_DATA:
				/* Not supported, clear buffer */
			default:
				session->state = FCGI_UNKNOWN_TYPE;
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

int32_t klunk_process_input(klunk_context_t *ctx, const char *data, const size_t len)
{
	int32_t length = 0;
	const char *ptr = 0;
	int32_t content_length = 0;
	int32_t padding_length = 0;
	int32_t bytes_left = 0;
	int32_t bytes_write = 0;
	int32_t result = E_SUCCESS;

	assert(len <= 0x7fffffff);
	length = (int32_t)len;
	ptr = data;

	/* Try to read header if neccesary */
	if (ctx->read_state == 0) {
		result = klunk_read_header(ctx->current_header, ptr, length);
		if (result == E_SUCCESS) {
			length -= sizeof(fcgi_record_header_t);
			ptr += sizeof(fcgi_record_header_t);
			ctx->read_bytes = 0;
			ctx->read_state = 1;
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
				ctx->read_bytes += bytes_write;
				/* Update counter */
				bytes_left = (content_length + padding_length) - ctx->read_bytes;
			}
			/* Check if all incoming data has been read */
			if (bytes_left == 0) {
				result = klunk_process_input_buffer(ctx);
				ctx->read_state = 0;
			}
		}	
	}
	return result;
}

int32_t klunk_process_data(klunk_context_t *ctx, const char *data, const size_t len)
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
	return klunk_process_input(ctx, data, len);
}

/**** Public functions ******/

klunk_context_t * klunk_create()
{
	klunk_context_t *ctx = 0;

	ctx = malloc(sizeof(klunk_context_t));
	if (ctx != 0) {
		ctx->sessions = llist_create(sizeof(klunk_request_t));
		if (ctx->sessions == 0) {
			free(ctx);
			ctx = 0;
		}
		else {
			llist_register_dtor(ctx->sessions
				, (llist_item_dtor)&klunk_request_destroy);
		}
	}
	if (ctx != 0) {
		ctx->input = buffer_create();
		if (ctx->input == 0) {
			llist_destroy(ctx->sessions);
			ctx->sessions = 0;
			free(ctx);
			ctx = 0;
		}
	}
	if (ctx != 0) {
		ctx->output = buffer_create();
		if (ctx->output == 0) {
			buffer_destroy(ctx->input);
			ctx->input = 0;
			llist_destroy(ctx->sessions);
			ctx->sessions = 0;
			free(ctx);
			ctx = 0;
		}
	}
	if (ctx != 0) {
		ctx->current_header = malloc(sizeof(fcgi_record_header_t));
		if (ctx->current_header == 0) {
			buffer_destroy(ctx->output);
			ctx->input = 0;
			buffer_destroy(ctx->input);
			ctx->input = 0;
			llist_destroy(ctx->sessions);
			ctx->sessions = 0;
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
			buffer_destroy(ctx->output);
			ctx->input = 0;
			buffer_destroy(ctx->input);
			ctx->input = 0;
			llist_destroy(ctx->sessions);
			ctx->sessions = 0;
			free(ctx);
			ctx = 0;
		}
	}
	if (ctx != 0) {
		ctx->file_descriptor = -1;
		ctx->read_state = 0;
		ctx->current_header->version = 0;
		ctx->current_header->type = 0;
		ctx->current_header->request_id = 0;
		ctx->current_header->content_length = 0;
		ctx->current_header->padding_length = 0;
	}
	return ctx;
}

void klunk_destroy(klunk_context_t *ctx)
{
	if (ctx != 0) {
		llist_destroy(ctx->sessions);
		ctx->sessions = 0;
		buffer_destroy(ctx->input);
		ctx->input = 0;
		buffer_destroy(ctx->output);
		ctx->output = 0;
		free(ctx->current_header);
		ctx->current_header = 0;
		free(ctx->read_buffer);
		ctx->read_buffer = 0;
		ctx->read_buffer_len = 0;
		free(ctx);
	}
}

int32_t klunk_set_file_descriptor(klunk_context_t *ctx, int32_t fd)
{
	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}
	if (fd < 0) {
		return E_INVALID_ARGUMENT;
	}
	ctx->file_descriptor = fd;
	return E_SUCCESS;
}

int32_t klunk_current_request(klunk_context_t *ctx)
{
	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}
	return ctx->current_header->request_id;
}

int32_t klunk_request_state(klunk_context_t *ctx, const uint16_t request_id)
{
	int32_t result = E_SUCCESS;
	klunk_request_t *session = 0;

	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}

	session = klunk_find_request(ctx, request_id);
	if (session == 0) {
		result = E_REQUEST_NOT_FOUND;
	}
	else {
		result = (int32_t)session->state;
	}
	return result;
}

int32_t klunk_read(klunk_context_t *ctx)
{
	int32_t result = E_SUCCESS;

	if (ctx == 0) {
		return E_INVALID_OBJECT;
	}
	if (ctx->file_descriptor < 0) {
		return E_INVALID_HANDLE;
	}
	result = read(ctx->file_descriptor, ctx->read_buffer, ctx->read_buffer_len);
	if (result >= 0) {
		result = klunk_process_data(ctx, ctx->read_buffer, result);
	}
	else {
		result = E_OS_ERROR | errno;
	}
	return result;
}

int32_t klunk_write(klunk_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;
	return result;
}

int32_t klunk_write_error(klunk_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;
	return result;
}

int32_t klunk_finish(klunk_context_t *ctx, const uint16_t request_id)
{
	int32_t result = E_SUCCESS;
	return result;
}
