/* Klunk a FCGI helper library, helper for helpers
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef KLUNK_UTILS_H
#define KLUNK_UTILS_H
 
#include <stdlib.h>
#include <stdint.h>

uint16_t klunk_size8b(const uint16_t size);

uint32_t klunk_murmur3_32(const void *data, const size_t len);

#endif /* KLUNK_UTILS_H */
