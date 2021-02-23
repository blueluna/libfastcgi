#include "utilities.h"
#include "errorcodes.h"

/* Calculate the size aligned to a 8-byte border */
uint16_t size8b(const uint16_t size)
{
	uint16_t s;
	if ((size % 8) == 0) {
		s = size;
	}
	else {
		s = ((size / 8) + 1) * 8;
	}
	return s;
}
