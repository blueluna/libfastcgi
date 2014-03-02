
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

enum {
	FCGI_REQUEST_COMPLETE	= 0,
	FCGI_CANT_MPX_CONN		= 1,
	FCGI_OVERLOADED			= 2,
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
	uint32_t        appStatus;
	uint8_t         protocolStatus;
	uint8_t         reserved[3];
} fcgi_record_end_t;

typedef struct fcgi_record_unknown_  {
	uint8_t         type;
	uint8_t         reserved[7];
} fcgi_record_unknown_t;

#endif /* FCGI_PROTOCOL_H */
