/* Klunk "private" functions
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef KLUNK_PRIVATE_H
#define KLUNK_PRIVATE_H

#include <stdint.h>
#include "klunk.h"

int32_t klunk_process_data(klunk_context_t *ctx, const char *data, const size_t len);

#endif /* KLUNK_PRIVATE_H */
