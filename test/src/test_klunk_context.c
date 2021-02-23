#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>

#include "testcase.h"
#include "fastcgi.h"
#include "request.h"
#include "parameter.h"
#include "errorcodes.h"
#include "test_klunk_context.h"

typedef struct {
	uint8_t version;
	uint8_t type;
	uint16_t request_id;
	uint16_t content_len;
	uint8_t padding_len;
	uint8_t reserved;
} fcgi_record_header;


typedef struct {
	fcgi_record_header 	header;
	const char 			*content;
} fcgi_record;

const char* type_as_string(uint8_t type)
{
	switch (type) {
		case 1:
			return "FCGI_BEGIN_REQUEST";
			break;
		case 2:
			return "FCGI_ABORT_REQUEST";
			break;
		case 3:
			return "FCGI_END_REQUEST";
			break;
		case 4:
			return "FCGI_PARAMS";
			break;
		case 5:
			return "FCGI_STDIN";
			break;
		case 6:
			return "FCGI_STDOUT";
			break;
		case 7:
			return "FCGI_STDERR";
			break;
		case 8:
			return "FCGI_DATA";
			break;
		case 9:
			return "FCGI_GET_VALUES";
			break;
		case 10:
			return "FCGI_GET_VALUES_RESULT";
			break;
		case 11:
			return "FCGI_UNKNOWN_TYPE";
			break;
		default:
			return "INVALID";
	}

}

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

int32_t generate_record_header(uint8_t *data, const int32_t len, uint8_t type
	, uint16_t request_id, uint16_t content_len, uint8_t padding_len)
{
	if (len < 8) {
		return 0;
	}
	uint16_t word;
	data[0] = 1;
	data[1] = type;
	word = htons(request_id);
	memcpy(data+2, &word, 2);
	word = htons(content_len);
	memcpy(data+4, &word, 2);
	data[6] = padding_len;
	return 8;
}


int32_t generate_begin(uint8_t *data, const int32_t len, uint16_t request_id)
{
	if (len < 16) {
		return 0;
	}
	int32_t pos = generate_record_header(data, len, 1, request_id, 8, 0);
	memcpy(data + pos, "\x00\x01\x00\x00\x00\x00\x00\x00", 8);
	return 16;
}

int32_t add_param(char *data, const int32_t len, const char *name, const char *value)
{
	char *ptr = data;
	int32_t used = 0;
	int32_t name_len = (int32_t)strlen(name);
	int32_t value_len = (int32_t)strlen(value);

	if (len < (name_len + value_len + 8)) {
		return 0;
	}

	if (name_len > 0x7f) {
		uint32_t l = (uint32_t)name_len | 0x80000000;
		l = htonl(l);
		memcpy(ptr, &l, 4);
		ptr += 4;
		used += 4;
	}
	else {
		*ptr = (char)name_len;
		ptr++;
		used++;
	}

	if (value_len > 0x7f) {
		uint32_t l = (uint32_t)value_len | 0x80000000;
		l = htonl(l);
		memcpy(ptr, &l, 4);
		ptr += 4;
		used += 4;
	}
	else {
		*ptr = (char)value_len;
		ptr++;
		used++;
	}

	memcpy(ptr, name, name_len);
	ptr += name_len;
	used += name_len;
	memcpy(ptr, value, value_len);
	used += value_len;

	return used;
}

int32_t generate_param(uint8_t *data, const int32_t len, uint16_t request_id
	, const char *params, const int32_t params_len)
{
	assert((params_len + 8) <= 0xffff);
	uint16_t size = size8b((uint16_t)((params_len+8)));
	if (len < size) {
		return 0;
	}
	uint8_t padding = (uint8_t)(size - (params_len + 8));
	int32_t pos = generate_record_header(data, len, 4, request_id, params_len, padding);
	memcpy(data + pos, params, params_len);
	memset(data + (pos + params_len), 0, padding);
	return size;
}

int32_t generate_stdin(uint8_t *data, const int32_t len, uint16_t request_id
	, const char *stdin, const int32_t stdin_len)
{
	assert((stdin_len + 8) <= 0xffff);
	uint16_t size = size8b((uint16_t)((stdin_len+8)));
	if (len < size) {
		return 0;
	}
	uint8_t padding = (uint8_t)(size - (stdin_len + 8));
	int32_t pos = generate_record_header(data, len, 5, request_id, stdin_len, padding);
	memcpy(data + pos, stdin, stdin_len);
	memset(data + (pos + stdin_len), 0, padding);
	return size;
}

int32_t parse_record(const char *data, const int32_t len, fcgi_record *rec)
{
	fcgi_record_header header;
	const char *ptr = data;
	int32_t total_length = 0;

	if (len >= (int32_t)sizeof(fcgi_record_header)) {
		memcpy(&header, ptr, sizeof(fcgi_record_header));
		ptr += sizeof(fcgi_record_header);
		header.request_id = ntohs(header.request_id);
		header.content_len = ntohs(header.content_len);
		total_length = sizeof(fcgi_record_header) + header.content_len
			+ header.padding_len;

		if (len >= total_length) {
			rec->header = header;
			rec->content = ptr;
		}
		else {
			total_length = 0;
		}
	}
	return total_length;
}

int32_t print_record(fcgi_record *rec)
{
	if (rec == 0) {
		return E_INVALID_ARGUMENT;
	}
#if 0
	printf("-- Record ----------------------------\n");
	printf("       Version: %u\n", rec->header.version);
	printf("          Type: %s (%u)\n", type_as_string(rec->header.type), rec->header.type);
	printf("    Request Id: %u\n", rec->header.request_id);
	printf("Content Length: %u\n", rec->header.content_len);
	printf("Padding Length: %u\n", rec->header.padding_len);
#endif
	return E_SUCCESS;
}

void klunk_context_test()
{
	int32_t result = E_SUCCESS;
	int32_t data_size = 0;
	int32_t params_size = 0;
	uint16_t request_id = 1;
	int str_result = 0;
	char *data = 0;
	char *params = 0;
	fastcgi_context_t *ctx = 0;
	fastcgi_request_t *request = 0;

	int32_t major, minor, patch;
	fastcgi_version(&major, &minor, &patch);
	TEST_ASSERT_EQUAL(major, 0);
	TEST_ASSERT_EQUAL(minor, 1);
	TEST_ASSERT_EQUAL(patch, 0);

	data = malloc(1024);
	assert(data != 0);
	params = malloc(1024);
	assert(params != 0);

	/* Test responses for null context */
	fastcgi_destroy(0);

	result = fastcgi_current_request_id(0);
	TEST_ASSERT_EQUAL(result, E_INVALID_OBJECT);

	result = fastcgi_request_state(0, 0);
	TEST_ASSERT_EQUAL(result, E_INVALID_OBJECT);

	result = fastcgi_read(0, 0, 0);
	TEST_ASSERT_EQUAL(result, E_INVALID_OBJECT);

	result = fastcgi_write_output(0, 0, 0, 0);
	TEST_ASSERT_EQUAL(result, E_INVALID_OBJECT);

	result = fastcgi_write_error(0, 0, 0, 0);
	TEST_ASSERT_EQUAL(result, E_INVALID_OBJECT);

	result = fastcgi_write(0, 0, 0, 0);
	TEST_ASSERT_EQUAL(result, E_INVALID_OBJECT);

	request = fastcgi_find_request(0, 0);
	TEST_ASSERT_EQUAL(request, 0);

	ctx = fastcgi_create();
	TEST_ASSERT_NOT_EQUAL(ctx, 0);
	if (ctx != 0) {
		/* Check state for newly initialized context */
		result = fastcgi_current_request_id(ctx);
		TEST_ASSERT_EQUAL(result, 0);

		result = fastcgi_request_state(ctx, 0);
		TEST_ASSERT_EQUAL(result, E_REQUEST_NOT_FOUND);

		result = fastcgi_read(ctx, 0, 0);
		TEST_ASSERT_EQUAL(result, E_INVALID_ARGUMENT);
		
		result = fastcgi_write_output(ctx, 0, 0, 0);
		TEST_ASSERT_EQUAL(result, E_REQUEST_NOT_FOUND);
		
		result = fastcgi_write_error(ctx, 0, 0, 0);
		TEST_ASSERT_EQUAL(result, E_REQUEST_NOT_FOUND);
		
		result = fastcgi_write(ctx, 0, 0, 0);
		TEST_ASSERT_EQUAL(result, E_REQUEST_NOT_FOUND);

		/* Insert a begin record to generate a request object */

		data_size = generate_begin((uint8_t*)data, 1024, request_id);

		result = fastcgi_read(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, data_size);

		result = fastcgi_current_request_id(ctx);
		TEST_ASSERT_EQUAL(result, request_id);

		result = fastcgi_request_state(ctx, result);
		TEST_ASSERT_EQUAL(result, FASTCGI_RS_NEW);
				
		result = fastcgi_write(ctx, 0, 0, request_id);
		TEST_ASSERT_EQUAL(result, E_INVALID_ARGUMENT);

		result = fastcgi_request_state(ctx, request_id);
		TEST_ASSERT_EQUAL(result, FASTCGI_RS_NEW);

		/* Run the same begin request again */

		result = fastcgi_read(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, E_REQUEST_DUPLICATE);

		result = fastcgi_request_state(ctx, request_id);
		TEST_ASSERT_EQUAL(result, FASTCGI_RS_NEW);

		/* Insert a param record */

		params_size = add_param(params, 1024, "hello", "world");
		data_size = generate_param((uint8_t*)data, 1024, request_id, params, params_size);

		result = fastcgi_read(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, data_size);

		result = fastcgi_request_state(ctx, request_id);
		TEST_ASSERT_EQUAL(result, (FASTCGI_RS_NEW | FASTCGI_RS_PARAMS));

		request = fastcgi_find_request(ctx, request_id);
		klunk_param_t *param = (klunk_param_t*)(request->params->items->data);
		
		str_result = strcmp(param->name, "hello");
		TEST_ASSERT_EQUAL(str_result, 0);
		
		str_result = strcmp(param->value, "world");
		TEST_ASSERT_EQUAL(str_result, 0);

		/* Insert another param record */

		params_size = add_param(params, 1024, "goodbye"
			, "lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
			);
		data_size = generate_param((uint8_t*)data, 1024, request_id, params, params_size);

		result = fastcgi_read(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, data_size);

		result = fastcgi_request_state(ctx, request_id);
		TEST_ASSERT_EQUAL(result, (FASTCGI_RS_NEW | FASTCGI_RS_PARAMS));

		data_size = generate_param((uint8_t*)data, 1024, request_id, 0, 0);

		result = fastcgi_read(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, data_size);

		result = fastcgi_request_state(ctx, request_id);
		TEST_ASSERT_EQUAL(result, (FASTCGI_RS_NEW | FASTCGI_RS_PARAMS
			| FASTCGI_RS_PARAMS_DONE));

		memcpy(params, "{\"hello\": \"world\"}", 18);
		data_size = generate_stdin((uint8_t*)data, 1024, request_id, params, 18);

		result = fastcgi_read(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, data_size);

		result = fastcgi_request_state(ctx, request_id);
		TEST_ASSERT_EQUAL(result, (FASTCGI_RS_NEW | FASTCGI_RS_PARAMS
			| FASTCGI_RS_PARAMS_DONE | FASTCGI_RS_STDIN));

		data_size = generate_stdin((uint8_t*)data, 1024, request_id, 0, 0);

		result = fastcgi_read(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, data_size);

		result = fastcgi_request_state(ctx, request_id);
		TEST_ASSERT_EQUAL(result, (FASTCGI_RS_NEW | FASTCGI_RS_PARAMS
			| FASTCGI_RS_PARAMS_DONE | FASTCGI_RS_STDIN | FASTCGI_RS_STDIN_DONE));

		data_size = buffer_used(request->content);
		memcpy(data, buffer_peek(request->content), data_size);
		data[data_size] = 0;
		params[18] = 0;
		str_result = strcmp(data, params);
		TEST_ASSERT_EQUAL(str_result, 0);

		fcgi_record rec;
		memcpy(params, "{\"hello\": \"world\"}", 18);

		fastcgi_write_output(ctx, request_id, params, 18);

		data_size = fastcgi_write(ctx, data, 1024, request_id);
		TEST_ASSERT_GT(data_size, 0);
		result = parse_record(data, data_size, &rec);
		if (result > 0) {
			print_record(&rec);
			TEST_ASSERT_EQUAL(rec.header.version, 1);
			TEST_ASSERT_EQUAL(rec.header.type, 6);
			TEST_ASSERT_EQUAL(rec.header.request_id, 1);
			TEST_ASSERT_EQUAL(rec.header.content_len, 18);
			TEST_ASSERT_EQUAL(rec.header.padding_len, size8b(18) - 18);
			result = memcmp(params, rec.content, rec.header.content_len);
			TEST_ASSERT_EQUAL(result, 0);
		}

		fastcgi_write_error(ctx, request_id, params, 18);

		result = fastcgi_write(ctx, data, 8, request_id);
		TEST_ASSERT_EQUAL(result, E_INVALID_SIZE);

		data_size = fastcgi_write(ctx, data, 1024, request_id);
		TEST_ASSERT_GT(data_size, 0);
		result = parse_record(data, data_size, &rec);
		if (result > 0) {
			print_record(&rec);
			TEST_ASSERT_EQUAL(rec.header.version, 1);
			TEST_ASSERT_EQUAL(rec.header.type, 7);
			TEST_ASSERT_EQUAL(rec.header.request_id, 1);
			TEST_ASSERT_EQUAL(rec.header.content_len, 18);
			TEST_ASSERT_EQUAL(rec.header.padding_len, size8b(18) - 18);
			result = memcmp(params, rec.content, rec.header.content_len);
			TEST_ASSERT_EQUAL(result, 0);
		}

		fastcgi_write_output(ctx, request_id, 0, 0);

		data_size = fastcgi_write(ctx, data, 1024, request_id);
		TEST_ASSERT_EQUAL(data_size, 0);

		result = fastcgi_finish(ctx, request_id);
		TEST_ASSERT_TRUE(result >= E_SUCCESS);

		result = fastcgi_write(ctx, data, 7, request_id);
		TEST_ASSERT_EQUAL(result, E_INVALID_SIZE);

		data_size = fastcgi_write(ctx, data, 1024, request_id);

		TEST_ASSERT_GT(data_size, 0);
		result = parse_record(data, data_size, &rec);
		if (result > 0) {
			print_record(&rec);
			TEST_ASSERT_EQUAL(rec.header.version, 1);
			TEST_ASSERT_EQUAL(rec.header.type, 7);
			TEST_ASSERT_EQUAL(rec.header.request_id, 1);
			TEST_ASSERT_EQUAL(rec.header.content_len, 0);
			TEST_ASSERT_EQUAL(rec.header.padding_len, 0);
		}

		result = fastcgi_write(ctx, data, 7, request_id);
		TEST_ASSERT_EQUAL(result, E_INVALID_SIZE);

		data_size = fastcgi_write(ctx, data, 1024, request_id);

		result = parse_record(data, data_size, &rec);
		if (result > 0) {
			print_record(&rec);
			TEST_ASSERT_EQUAL(rec.header.version, 1);
			TEST_ASSERT_EQUAL(rec.header.type, 6);
			TEST_ASSERT_EQUAL(rec.header.request_id, 1);
			TEST_ASSERT_EQUAL(rec.header.content_len, 0);
			TEST_ASSERT_EQUAL(rec.header.padding_len, 0);
		}

		result = fastcgi_write(ctx, data, 15, request_id);
		TEST_ASSERT_EQUAL(result, E_INVALID_SIZE);

		data_size = fastcgi_write(ctx, data, 1024, request_id);

		result = parse_record(data, data_size, &rec);
		if (result > 0) {
			print_record(&rec);
			TEST_ASSERT_EQUAL(rec.header.version, 1);
			TEST_ASSERT_EQUAL(rec.header.type, 3);
			TEST_ASSERT_EQUAL(rec.header.request_id, 1);
			TEST_ASSERT_EQUAL(rec.header.content_len, 8);
			TEST_ASSERT_EQUAL(rec.header.padding_len, 0);
			memcpy(params, "\0\0\0\0\0\0\0\0", 8);
			result = memcmp(params, rec.content, rec.header.content_len);
			TEST_ASSERT_EQUAL(result, 0);
		}

		result = fastcgi_write(ctx, data, 1024, request_id);

		TEST_ASSERT_EQUAL(result, E_REQUEST_NOT_FOUND);

		request = fastcgi_find_request(ctx, request_id);
		TEST_ASSERT_EQUAL(request, 0);

		fastcgi_destroy(ctx);
		fastcgi_destroy(0);
	}

	free(data);
	free(params);
}
