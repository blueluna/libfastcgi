
#ifndef FCGI_H
#define FCGI_H

#include <stdint.h>
#include <string.h>
#include "llist.h"
#include "buffer.h"
#include "fcgi_protocol.h"
#include "fcgi_session.h"

enum {
	FCGI_NEW_REQUEST = 1,
	FCGI_PARAMS_DONE,
	FCGI_STDIN_DONE,
};

typedef struct fcgi_context_  {
	llist_t					*sessions;
	buffer_t				*input;
	buffer_t				*output;
	fcgi_record_header_t	*current_header;
	uint8_t					read_state;
	int32_t					read_bytes;
} fcgi_context_t;

fcgi_context_t* fcgi_create();

void fcgi_destroy(fcgi_context_t *ctx);

int32_t fcgi_current_request(fcgi_context_t *ctx);
int32_t fcgi_request_state(fcgi_context_t *ctx, const uint16_t request_id);

int32_t fcgi_read(fcgi_context_t *ctx, const char *data, const size_t len);

int32_t fcgi_write(fcgi_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len);

int32_t fcgi_error(fcgi_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len);

int32_t fcgi_finish(fcgi_context_t *ctx, const uint16_t request_id);

fcgi_session_t* fcgi_find_session(fcgi_context_t *ctx, const uint16_t id);

#endif /* FCGI_H */
