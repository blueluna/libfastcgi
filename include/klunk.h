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

/* Create a klunk context used for handling FCGI requests */
klunk_context_t* klunk_create();

/* Destroy the klunk context */
void klunk_destroy(klunk_context_t *ctx);

/* Set the file descriptor that should be used when reading and writing
 * FCGI requests/data.
 */
int32_t klunk_set_file_descriptor(klunk_context_t *ctx, int32_t fd);

/* Get the registered file descriptor 
 * Negative return value means error or that the file descriptor is
 * uninitialized.
 */
int32_t klunk_get_file_descriptor(klunk_context_t *ctx);

/* Get the current request id.
 * Negative return value means error.
 */
int32_t klunk_current_request(klunk_context_t *ctx);

/* Get the request state for the request with the supplied id.
 * Negative return value means error.
 */
int32_t klunk_request_state(klunk_context_t *ctx, const uint16_t request_id);

/* Read data from the file descriptor and try to generate requests.
 * Return value can be one of following:
 * KLUNK_NEW_REQUEST, A new request was found.
 * KLUNK_PARAMS_DONE, A request have received all parameters.
 * KLUNK_STDIN_DONE, A request have received all data.
 * Negative return value means error.
 */
int32_t klunk_read(klunk_context_t *ctx);

/* Write data to the web server through the file descriptor.
 * Negative return value means error.
 */
int32_t klunk_write(klunk_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len);

/* Write error data to the web server through the file descriptor.
 * Negative return value means error.
 */
int32_t klunk_write_error(klunk_context_t *ctx, const uint16_t request_id
	, const char *data, const size_t len);

/* Finish the requests and remove the request object.
 * Negative return value means error.
 */
int32_t klunk_finish(klunk_context_t *ctx, const uint16_t request_id);

/* Find and return the request with the supplied id. return zero if the 
 * request object wasn't found.
 */
klunk_request_t* klunk_find_request(klunk_context_t *ctx, const uint16_t id);

#endif /* KLUNK_H */
