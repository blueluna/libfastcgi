
#ifndef FCGI_SESSION_H
#define FCGI_SESSION_H

#include <stdint.h>
#include "llist.h"
#include "buffer.h"

typedef struct fcgi_session_  {
	uint16_t        id;
	uint16_t        role;
	uint8_t         flags;
	uint8_t         state;
	llist_t			*params;
	buffer_t		*content;
} fcgi_session_t;

fcgi_session_t* fcgi_session_create();
void fcgi_session_destroy(fcgi_session_t *session);

#endif /* FCGI_SESSION_H */
