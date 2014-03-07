/* A growing memory buffer
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT licence.
 */

#include "buffer.h"
#include "errorcodes.h"
#include <stdlib.h>

buffer_t* buffer_create()
{
	buffer_t *buf = malloc(sizeof(buffer_t));
	if (buf != 0) {
		buf->data = malloc(1024);
		if (buf->data == 0) {
			free(buf);
			buf = 0;
		}
		else {
			buf->size = 1024;
			buf->used = 0;
		}
	}
	return buf;
}

void buffer_destroy(buffer_t *buf)
{
	if (buf != 0) {
		if (buf->data != 0) {
			free(buf->data);
			buf->data = 0;
			buf->size = 0;
			buf->used = 0;
		}
		free(buf);
	}
}

size_t buffer_size(buffer_t *buf)
{
	if (buf != 0) {
		return buf->size;
	}
	return 0;
}

size_t buffer_used(buffer_t *buf)
{
	if (buf != 0) {
		return buf->used;
	}
	return 0;
}

size_t buffer_free(buffer_t *buf)
{
	if (buf != 0) {
		return (buf->size - buf->used);
	}
	return 0;
}

int32_t buffer_resize(buffer_t *buf, const size_t minsize)
{
	if (buf == 0) {
		return E_INVALID_ARGUMENT;
	}
	size_t new_buffer_size = ((minsize / 1024) + 1) * 1024;
	if (new_buffer_size < buf->size) {
		return buf->size;
	}
	char *new_buffer_data = malloc(new_buffer_size);
	if (new_buffer_data == 0) {
		return E_MEMORY_ALLOCATION_FAILED;
	}
	if (buf->used > 0) {
		memcpy(new_buffer_data, buf->data, buf->used);
	}
	free(buf->data);
	buf->size = new_buffer_size;
	buf->data = new_buffer_data;
	return new_buffer_size;
}

int32_t buffer_read(buffer_t *buf, char *data, const size_t len)
{
	if (buf == 0) {
		return E_INVALID_ARGUMENT;
	}
	if (buf->used == 0) {
		return 0;
	}
	/* Use the smallest of len and buf->used. */
	size_t read_len = len > buf->used ? buf->used : len;
	char * src_ptr = buf->data;
	char * dst_ptr = data;
	if (dst_ptr != 0) {
		memcpy(dst_ptr, src_ptr, read_len);
	}
	size_t left = buf->used - read_len;
	if (left > 0) {
		memmove(buf->data, buf->data+read_len, left);
	}
	buf->used -= read_len;
	return read_len;

}

int32_t buffer_write(buffer_t *buf, const char *data, const size_t len)
{
	if (buf == 0) {
		return E_INVALID_ARGUMENT;
	}
	if (len > buffer_free(buf)) {
		int32_t result = buffer_resize(buf, buf->used + len);
		if (result < 0) {
			return result;
		}
	}
	char *dst_ptr = buf->data + buf->used;
	memcpy(dst_ptr, data, len);
	buf->used += len;
	return len;
}

char* buffer_peek(buffer_t *buf)
{
	if (buf == 0) {
		return 0;
	}
	if (buf->size == 0) {
		return 0;
	}
	return buf->data;
}

void buffer_clear(buffer_t *buf)
{
	if (buf == 0) {
		return;
	}
	buf->used = 0;
}

int32_t buffer_reset(buffer_t *buf)
{
	if (buf == 0) {
		return E_INVALID_ARGUMENT;
	}
	if (buf->data != 0) {
		free(buf->data);
		buf->data = 0;
		buf->size = 0;
		buf->used = 0;
	}
	buf->data = malloc(1024);
	if (buf->data != 0) {
		buf->size = 1024;
		buf->used = 0;
	}
	else {
		return -1;
	}
	return 0;
}

int32_t buffer_reserve(buffer_t *buf, const size_t len)
{
	return buffer_resize(buf, len);
}