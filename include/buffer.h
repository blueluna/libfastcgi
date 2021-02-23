/* A growing memory buffer
 * (C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#ifndef ES_BUFFER_H
#define ES_BUFFER_H

#include <stdint.h>
#include <string.h>

typedef struct buffer_ {
	size_t	size;
	size_t	used;
	char	*data;
} buffer_t;

/* Create a buffer */
buffer_t*	buffer_create();

/* Destroy the buffer */
void		buffer_destroy(buffer_t *buf);

/* Get the number of allocated bytes in the buffer */
size_t		buffer_size(buffer_t *buf);

/* Get the number of used bytes in the buffer */
size_t		buffer_used(buffer_t *buf);

/* Get the number of free bytes in the buffer */
size_t		buffer_free(buffer_t *buf);

/* Read data from the buffer, this removes the data from the buffer */
int32_t		buffer_read(buffer_t *buf, char *data, const size_t len);

/* Write data to the buffer, if the data won't fit this function will try to
 * allocate more memory.
 */
int32_t		buffer_write(buffer_t *buf, const char *data, const size_t len);

/* Look at the data in the buffer */
char* 		buffer_peek(buffer_t *buf);

/* Clear the data in the buffer while retain the memory */
void		buffer_clear(buffer_t *buf);

/* Clear the data in the buffer and reset the memory allocated to the minimum
 */
int32_t		buffer_reset(buffer_t *buf);

int32_t		buffer_reserve(buffer_t *buf, const size_t len);

#endif /* ES_BUFFER_H */
