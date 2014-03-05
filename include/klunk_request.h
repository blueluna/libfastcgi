/* Klunk request, handling the state of a FCGI request.
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef KLUNK_REQUEST_H
#define KLUNK_REQUEST_H

#include <stdint.h>
#include "llist.h"
#include "buffer.h"

typedef struct klunk_request_  {
	uint16_t        id;
	uint16_t        role;
	uint8_t         flags;
	uint8_t         state;
	llist_t			*params;
	buffer_t		*content;
} klunk_request_t;

klunk_request_t* klunk_request_create();
void klunk_request_destroy(klunk_request_t *session);

#endif /* KLUNK_REQUEST_H */
