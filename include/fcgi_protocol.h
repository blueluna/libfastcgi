/* FCGI protocol constants and structures
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef FCGI_PROTOCOL_H
#define FCGI_PROTOCOL_H

#include <stdint.h>

enum {
	FCGI_VERSION_1		= 1
};

enum {
	FCGI_BEGIN_REQUEST		= 1,
	FCGI_ABORT_REQUEST		= 2,
	FCGI_END_REQUEST		= 3,
	FCGI_PARAMS				= 4,
	FCGI_STDIN				= 5,
	FCGI_STDOUT				= 6,
	FCGI_STDERR				= 7,
	FCGI_DATA				= 8,
	FCGI_GET_VALUES			= 9,
	FCGI_GET_VALUES_RESULT	= 10,
	FCGI_UNKNOWN_TYPE		= 11,
	FCGI_MAXTYPE			= FCGI_UNKNOWN_TYPE
};

enum {
	FCGI_RESPONDER			= 1,
	FCGI_AUTHORIZER			= 2,
	FCGI_FILTER				= 3
};

/* protocolStatus codes */
enum {
	/* Request finished without error */
	FCGI_REQUEST_COMPLETE	= 0,
	/* Request rejected because of too many multiplexed connections */
	FCGI_CANT_MPX_CONN		= 1,
	/* Request rejected because of resource exhaustion (like DB connections) */
	FCGI_OVERLOADED			= 2,
	/* Request rejected because the role cannot be handled */
	FCGI_UNKNOWN_ROLE		= 3
};

#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

typedef struct fcgi_record_header_  {
	uint8_t     version;
	uint8_t     type;
	uint16_t    request_id;
	uint16_t    content_length;
	uint8_t     padding_length;
	uint8_t     reserved;
} fcgi_record_header_t;

typedef struct fcgi_record_begin_  {
	uint16_t        role;
	uint8_t         flags;
	uint8_t         reserved[5];
} fcgi_record_begin_t;

typedef struct fcgi_record_end_  {
	/* appStatus is role dependand
	 * RESPONDER: CGI exit code
	 * AUTHORIZER: ???
	 * FILTER: CGI exit code
	 */
	uint32_t        appStatus;
	/* See enum above */
	uint8_t         protocolStatus;
	uint8_t         reserved[3];
} fcgi_record_end_t;

typedef struct fcgi_record_unknown_  {
	uint8_t         type;
	uint8_t         reserved[7];
} fcgi_record_unknown_t;

#endif /* FCGI_PROTOCOL_H */
