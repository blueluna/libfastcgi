#include "fcgi.h"
#include "errorcodes.h"
#include "fcgi_session.h"
#include "fcgi_param.h"
#include <assert.h>
#include <stdlib.h>
#include <arpa/inet.h>

uint16_t fcgi_size8b(const uint16_t size)
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

fcgi_session_t* fcgi_find_session(fcgi_context_t *ctx, const uint16_t id)
{
	if (ctx == 0 || ctx->sessions == 0 || ctx->sessions->items == 0) {
		return 0;
	}
	fcgi_session_t* session = 0;
	fcgi_session_t* item = 0;
	llist_item_t *ptr = ctx->sessions->items;
	while (ptr != 0) {
		item = (fcgi_session_t*)(ptr->data);
		if (item->id == id) {
			session = (fcgi_session_t*)(ptr->data);
			break;
		}
		ptr = ptr->next;
	}
	return session;
}

int32_t fcgi_read_header(fcgi_record_header_t *header, const char *data
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

int32_t fcgi_begin_request(fcgi_context_t *ctx, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;
	fcgi_record_begin_t record = {0};
	fcgi_session_t *session = 0;

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
		session = fcgi_session_create();
		session->id = ctx->current_header->request_id;
		session->role = record.role;
		session->flags = record.flags;
		session->state = FCGI_BEGIN_REQUEST;
		result = llist_add(ctx->sessions, session, sizeof(fcgi_session_t));
	}
	return result;
}

int32_t fcgi_params(fcgi_session_t *session, const char *data, const size_t len)
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

int32_t fcgi_stdin(fcgi_session_t *session, const char *data, const size_t len)
{
	int32_t result = E_SUCCESS;

	session->state = FCGI_STDIN;
	result = buffer_write(session->content, data, len);

	return result;
}

int32_t fcgi_process_input_buffer(fcgi_context_t *ctx)
{
	int32_t result = E_SUCCESS;
	int32_t buffer_length = 0;
	const char *buffer_data = 0;
	int32_t bytes_used = 0;
	fcgi_session_t *session = 0;

	session = fcgi_find_session(ctx, ctx->current_header->request_id);
	if (session == 0) {
		if (ctx->current_header->type != FCGI_BEGIN_REQUEST) {
			result = E_FCGI_SESSION_NOT_FOUND;
		}
	}
	else {
		if (ctx->current_header->type == FCGI_BEGIN_REQUEST) {
			result = E_FCGI_SESSION_DUPLICATE;
		}
	}

	if (result == E_SUCCESS) {

		buffer_length = buffer_used(ctx->input);
		buffer_data = buffer_peek(ctx->input);
		bytes_used = 0;
		switch (ctx->current_header->type) {
			case FCGI_BEGIN_REQUEST:
				result = fcgi_begin_request(ctx, buffer_data, buffer_length);
				if (result >= 0) {
					result = FCGI_NEW_REQUEST;
				}
				bytes_used = buffer_length;
				break;
			case FCGI_PARAMS:
				result = fcgi_params(session, buffer_data, buffer_length);
				if (result >= 0) {
					bytes_used = result;
					if (ctx->current_header->content_length == 0) {
						/* We have received all PARAMS data */
						result = FCGI_PARAMS_DONE;
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
				result = fcgi_stdin(session, buffer_data, buffer_length);
				if (result >= 0) {
					bytes_used = result;
					if (ctx->current_header->content_length == 0) {
						/* We have received all STDIN data */
						result = FCGI_STDIN_DONE;
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

int32_t fcgi_process_input(fcgi_context_t *ctx, const char *data, const size_t len)
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
		result = fcgi_read_header(ctx->current_header, ptr, length);
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
				result = fcgi_process_input_buffer(ctx);
				ctx->read_state = 0;
			}
		}	
	}
	return result;
}

/**** Public functions ******/

fcgi_context_t * fcgi_create()
{
	fcgi_context_t *ctx = 0;

	ctx = malloc(sizeof(fcgi_context_t));
	if (ctx != 0) {
		ctx->sessions = llist_create(sizeof(fcgi_session_t));
		if (ctx->sessions == 0) {
			free(ctx);
			ctx = 0;
		}
		else {
			llist_register_dtor(ctx->sessions
				, (llist_item_dtor)&fcgi_session_destroy);
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
		ctx->read_state = 0;
		ctx->current_header->version = 0;
		ctx->current_header->type = 0;
		ctx->current_header->request_id = 0;
		ctx->current_header->content_length = 0;
		ctx->current_header->padding_length = 0;
	}
	return ctx;
}

void fcgi_destroy(fcgi_context_t *ctx)
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
		free(ctx);
	}
}

int32_t fcgi_current_request(fcgi_context_t *ctx)
{
	if (ctx == 0) {
		return E_INVALID_ARGUMENT;
	}
	return ctx->current_header->request_id;
}

int32_t fcgi_request_state(fcgi_context_t *ctx, const uint16_t request_id)
{
	int32_t result = E_SUCCESS;
	fcgi_session_t *session = 0;

	if (ctx == 0) {
		return E_INVALID_ARGUMENT;
	}

	session = fcgi_find_session(ctx, request_id);
	if (session == 0) {
		result = E_FCGI_SESSION_NOT_FOUND;
	}
	else {
		result = (int32_t)session->state;
	}
	return result;
}

int32_t fcgi_read(fcgi_context_t *ctx, const char *data, const size_t len)
{
	if (ctx == 0 || data == 0 || len == 0) {
		return E_INVALID_ARGUMENT;
	}
	return fcgi_process_input(ctx, data, len);
}

int32_t fcgi_write(fcgi_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len)
{
}

int32_t fcgi_finish(fcgi_context_t *ctx, const uint16_t request_id)
{
}