#include <stdint.h>
#include <assert.h>
#include <arpa/inet.h>

#include "testcase.h"
#include "klunk.h"
#include "klunk_request.h"
#include "fcgi_param.h"
#include "errorcodes.h"
#include "test_klunk_context.h"

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

void klunk_context_test()
{
	int32_t result = E_SUCCESS;
	int32_t data_size = 0;
	int32_t params_size = 0;
	int str_result = 0;
	char *data = 0;
	char *params = 0;
	klunk_context_t* ctx = 0;

	data = malloc(1024);
	assert(data != 0);
	params = malloc(1024);
	assert(params != 0);

	ctx = klunk_create();
	TEST_ASSERT_NOT_EQUAL(ctx, 0);
	if (ctx != 0) {
		result = klunk_current_request(ctx);
		TEST_ASSERT_EQUAL(result, 0);
		result = klunk_request_state(ctx, 0);
		TEST_ASSERT_EQUAL(result, E_REQUEST_NOT_FOUND);

		data_size = generate_begin((uint8_t*)data, 1024, 1);

		result = klunk_process_data(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, KLUNK_NEW_REQUEST);

		result = klunk_current_request(ctx);
		TEST_ASSERT_EQUAL(result, 1);

		result = klunk_request_state(ctx, result);
		TEST_ASSERT_EQUAL(result, FCGI_BEGIN_REQUEST);

		result = klunk_process_data(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, E_REQUEST_DUPLICATE);

		params_size = add_param(params, 1024, "hello", "world");
		data_size = generate_param((uint8_t*)data, 1024, 1, params, params_size);

		result = klunk_process_data(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, E_SUCCESS);

		klunk_request_t *request = klunk_find_request(ctx, 1);
		fcgi_param_t *param = (fcgi_param_t*)(request->params->items->data);
		
		str_result = strcmp(param->name, "hello");
		TEST_ASSERT_EQUAL(str_result, 0);
		
		str_result = strcmp(param->value, "world");
		TEST_ASSERT_EQUAL(str_result, 0);

		params_size = add_param(params, 1024, "goodbye"
			, "lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
			);
		data_size = generate_param((uint8_t*)data, 1024, 1, params, params_size);

		result = klunk_process_data(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, E_SUCCESS);

		data_size = generate_param((uint8_t*)data, 1024, 1, 0, 0);

		result = klunk_process_data(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, KLUNK_PARAMS_DONE);

		memcpy(params, "{\"hello\": \"world\"}", 18);
		data_size = generate_stdin((uint8_t*)data, 1024, 1, params, 18);

		result = klunk_process_data(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, E_SUCCESS);

		data_size = generate_stdin((uint8_t*)data, 1024, 1, 0, 0);

		result = klunk_process_data(ctx, data, data_size);
		TEST_ASSERT_EQUAL(result, KLUNK_STDIN_DONE);

		data_size = buffer_used(request->content);
		memcpy(data, buffer_peek(request->content), data_size);
		data[data_size] = 0;
		params[18] = 0;
		str_result = strcmp(data, params);
		TEST_ASSERT_EQUAL(str_result, 0);

		klunk_destroy(ctx);
	}

	free(data);
	free(params);
}
