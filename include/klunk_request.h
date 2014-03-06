/* Klunk request, handling the state of a FCGI request.
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef KLUNK_REQUEST_H
#define KLUNK_REQUEST_H

#include <stdint.h>
#include "llist.h"
#include "buffer.h"

enum {
	KLUNK_RS_INIT			= 0,
	KLUNK_RS_NEW			= 1,
	KLUNK_RS_PARAMS			= (1 << 1),
	KLUNK_RS_PARAMS_DONE	= (1 << 2),
	KLUNK_RS_STDIN			= (1 << 3),
	KLUNK_RS_STDIN_DONE		= (1 << 4),
	KLUNK_RS_STDOUT			= (1 << 5),
	KLUNK_RS_STDOUT_DONE	= (1 << 6),
	KLUNK_RS_STDERR			= (1 << 7),
	KLUNK_RS_STDERR_DONE	= (1 << 8),
	KLUNK_RS_FINISHED		= (1 << 9)
};

typedef struct klunk_request_  {
	uint16_t        id;
	uint16_t        role;
	uint8_t         flags;
	uint8_t         reserved;
	uint16_t		state;
	llist_t			*params;
	buffer_t		*content;
} klunk_request_t;

/* Create a request "object" */
klunk_request_t* klunk_request_create();

/* Destroy the request "object" */
void klunk_request_destroy(klunk_request_t *request);

/* Get request state */
int32_t klunk_request_get_state(klunk_request_t *request, const uint16_t mask);

/* Set request state */
int32_t klunk_request_set_state(klunk_request_t *request, const uint16_t state);

#endif /* KLUNK_REQUEST_H */
