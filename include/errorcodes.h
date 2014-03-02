/* Error codes
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT licence.
 */

 #ifndef ES_ERRORCODES_H
 #define ES_ERRORCODES_H

enum {
	E_SUCCESS						= 0,
	E_INVALID_ARGUMENT				= -1,
	E_INVALID_SIZE					= -2,
	E_INVALID_TYPE					= -3,
	E_INVALID_OBJECT				= -4,
	E_NOT_FOUND						= -5,
	E_DUPLICATE						= -6,
	E_READ_FAILED					= -7,
	E_WRITE_FAILED					= -8,
	E_SYSTEM_ERRORS					= -500,
	E_MEMORY_ALLOCATION_FAILED		= (E_SYSTEM_ERRORS),
	E_FCGI_ERRORS                   = -1000,
	E_FCGI_INVALID_DATA             = (E_FCGI_ERRORS - 1),
	E_FCGI_INVALID_ROLE             = (E_FCGI_ERRORS - 2),
	E_INVALID_FCGI_CONTEXT			= (E_FCGI_ERRORS - 10),
	E_FCGI_SESSION_INVALID			= (E_FCGI_ERRORS - 20),
	E_FCGI_SESSION_NOT_FOUND		= (E_FCGI_ERRORS - 21),
	E_FCGI_SESSION_DUPLICATE		= (E_FCGI_ERRORS - 22),
};

#endif /* ES_ERRORCODES_H */
