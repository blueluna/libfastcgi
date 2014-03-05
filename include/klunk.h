/* Klunk a FCGI helper library
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef KLUNK_H
#define KLUNK_H

#include <stdint.h>
#include "llist.h"
#include "buffer.h"
#include "fcgi_protocol.h"
#include "klunk_request.h"

enum {
	KLUNK_NEW_REQUEST = 1,
	KLUNK_PARAMS_DONE,
	KLUNK_STDIN_DONE,
};

typedef struct klunk_context_  {
	int32_t					file_descriptor;
	llist_t					*requests;
	buffer_t				*input;
	buffer_t				*output;
	fcgi_record_header_t	*current_header;
	uint8_t					read_state;
	int32_t					read_bytes;
	char					*read_buffer;
	int32_t					read_buffer_len;
} klunk_context_t;

klunk_context_t* klunk_create();

void klunk_destroy(klunk_context_t *ctx);

int32_t klunk_set_file_descriptor(klunk_context_t *ctx, int32_t fd);

int32_t klunk_current_request(klunk_context_t *ctx);
int32_t klunk_request_state(klunk_context_t *ctx, const uint16_t request_id);

int32_t klunk_read(klunk_context_t *ctx);

int32_t klunk_process_data(klunk_context_t *ctx, const char *data, const size_t len);

int32_t klunk_write(klunk_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len);

int32_t klunk_write_error(klunk_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len);

int32_t klunk_finish(klunk_context_t *ctx, const uint16_t request_id);

klunk_request_t* klunk_find_request(klunk_context_t *ctx, const uint16_t id);

#endif /* KLUNK_H */
