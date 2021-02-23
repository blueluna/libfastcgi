/* Klunk a FCGI helper library
 * (C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef FASTCGI_H
#define FASTCGI_H

#include <stdint.h>
#include "llist.h"
#include "buffer.h"
#include "protocol.h"
#include "request.h"
#include "parameter.h"

void fastcgi_version(int32_t *version_major, int32_t *version_minor
	, int32_t *version_patch);

typedef struct fastcgi_context_  {
	llist_t					*requests;
	buffer_t				*input;
	fcgi_record_header_t	*current_header;
	uint8_t					read_state;
	int32_t					read_bytes;
	char					*read_buffer;
	int32_t					read_buffer_len;
} fastcgi_context_t;

/* Create a klunk context used for handling FCGI requests */
fastcgi_context_t* fastcgi_create();

/* Destroy the klunk context */
void fastcgi_destroy(fastcgi_context_t *ctx);

/* Get the current request id.
 * Negative return value means error.
 */
int32_t fastcgi_current_request_id(fastcgi_context_t *ctx);

/* Get the request state for the request with the supplied id.
 * Negative return value means error.
 */
int32_t fastcgi_request_state(fastcgi_context_t *ctx, const uint16_t request_id);

/* Read data from the file descriptor and try to generate requests.
 * Returns number of bytes used.
 * Negative return value means error.
 */
int32_t fastcgi_read(fastcgi_context_t *ctx
	, const char *input, const size_t input_len);

/* Write data to the web server through the file descriptor.
 * Negative return value means error.
 */
int32_t fastcgi_write_output(fastcgi_context_t *ctx
	, const uint16_t request_id
	, const char *input, const size_t input_len);

/* Write error data to the web server through the file descriptor.
 * Negative return value means error.
 */
int32_t fastcgi_write_error(fastcgi_context_t *ctx
	, const uint16_t request_id
	, const char *input, const size_t input_len);

/* 
 * Negative return value means error.
 */
int32_t fastcgi_finish(fastcgi_context_t *ctx, const uint16_t request_id);

/* 
 * Negative return value means error.
 */
int32_t fastcgi_write(fastcgi_context_t *ctx
	, char *output, const size_t output_len
	, const uint16_t request_id);

/* Find and return the request with the supplied id. return zero if the 
 * request object wasn't found.
 */
fastcgi_request_t* fastcgi_find_request(fastcgi_context_t *ctx, const uint16_t id);

/* Find and return the request with the supplied id, removing it from the
 * internal list. return zero if the request object wasn't found.
 */
fastcgi_request_t* fastcgi_take_request(fastcgi_context_t *ctx, const uint16_t id);

#endif /* FASTCGI_H */
