/* Klunk request, handling the state of a FCGI request.
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef FASTCGI_REQUEST_H
#define FASTCGI_REQUEST_H

#include <stdint.h>
#include "llist.h"
#include "buffer.h"

/* Request state flags
 */
enum {
	FASTCGI_RS_INIT				= 0,
	FASTCGI_RS_NEW				= 1,
	FASTCGI_RS_PARAMS				= (1 << 1),
	FASTCGI_RS_PARAMS_DONE		= (1 << 2),
	FASTCGI_RS_STDIN				= (1 << 3),
	FASTCGI_RS_STDIN_DONE			= (1 << 4),
	FASTCGI_RS_STDOUT				= (1 << 5),
	FASTCGI_RS_STDOUT_DONE		= (1 << 6),
	FASTCGI_RS_STDERR				= (1 << 7),
	FASTCGI_RS_STDERR_DONE		= (1 << 8),
	FASTCGI_RS_FINISH				= (1 << 9),
	FASTCGI_RS_FINISHED			= (1 << 10)
};

typedef struct fastcgi_request_  {
	uint16_t        id;
	uint16_t        role;
	uint16_t		state;
	uint8_t         flags;
	uint8_t         protocol_status;
	uint32_t		app_status;
	llist_t			*params;
	buffer_t		*content;
	buffer_t		*output;
	buffer_t		*error;
} fastcgi_request_t;

/* Create a request "object" */
fastcgi_request_t* fastcgi_request_create();

/* Destroy the request "object" */
void fastcgi_request_destroy(fastcgi_request_t *request);

/* Reset the request "object" */
void fastcgi_request_reset(fastcgi_request_t *request);

/* Get request state */
int32_t fastcgi_request_get_state(fastcgi_request_t *request, const uint16_t mask);

/* Set request state */
int32_t fastcgi_request_set_state(fastcgi_request_t *request, const uint16_t state);

/* Add parameter
 * Negative return value means error.
 */
int32_t fastcgi_request_parameter_add(fastcgi_request_t *request
	, const char *name, const size_t name_len
	, const char *value, const size_t value_len);

/* Write data that shall be sent to the server.
 * Negative return value means error.
 */
int32_t fastcgi_request_write_output(fastcgi_request_t *request
	, const char *input, const size_t input_len);

/* Write error data that shall be sent to the server.
 * Negative return value means error.
 */
int32_t fastcgi_request_write_error(fastcgi_request_t *request
	, const char *input, const size_t input_len);

/* Mark the request as finished.
 * Negative return value means error.
 */
int32_t fastcgi_request_finish(fastcgi_request_t *request
	, const uint32_t app_status, const uint8_t protocol_status);

/* Generate FCGI records from request.
 * Negative return value means error.
 */
int32_t fastcgi_request_output(fastcgi_request_t *request
	, char *output, const size_t output_len);

#endif /* FASTCGI_REQUEST_H */
