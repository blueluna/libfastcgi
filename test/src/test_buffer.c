#include "testcase.h"
#include "buffer.h"
#include "errorcodes.h"
#include "test_buffer.h"

void buffer_test()
{
	int32_t result = 0;
	int32_t i = 0;
	size_t data_len = 3072;
	char *data1 = malloc(data_len);
	if (data1 == 0) {
		return;
	}
	char *data2 = malloc(data_len);
	if (data2 == 0) {
		return;
	}
	for (i = 0; i < (int32_t)data_len; i++) {
		data1[i] = (char)(i % 256);
		data2[i] = ~(char)(i % 256);
	}
	buffer_t* buffer = buffer_create();
	TEST_ASSERT_NOT_EQUAL(buffer, 0);
	if (buffer != 0) {
		TEST_ASSERT_EQUAL(buffer_size(buffer), 1024);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 0);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);
		
		buffer_clear(buffer);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 1024);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 0);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);
		
		result = buffer_read(buffer, data1, 1024);
		TEST_ASSERT_EQUAL(result, 0);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 1024);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 0);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);
		
		result = buffer_write(buffer, data1, data_len);
		TEST_ASSERT_EQUAL(result, (int32_t)data_len);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), data_len);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);
		
		result = buffer_read(buffer, data2, data_len);
		TEST_ASSERT_EQUAL(result, (int32_t)data_len);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 0);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 4096);
		for (i = 0; i < (int32_t)data_len; i++) {
			TEST_ASSERT_EQUAL(data1[i], data2[i]);
		}

		/* Test that clear clears the usage, write some */
		
		result = buffer_write(buffer, data1, data_len);
		TEST_ASSERT_EQUAL(result, (int32_t)data_len);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), data_len);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);

		/* Test that clear clears the usage */

		buffer_clear(buffer);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 0);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 4096);

		/* Test that reset clears the storage, write some */
		
		result = buffer_write(buffer, data1, data_len);
		TEST_ASSERT_EQUAL(result, (int32_t)data_len);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), data_len);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);

		/* Test that reset clears the storage */

		buffer_reset(buffer);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 1024);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 0);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);

		/* Test that peeking on an empty buffer returns a null pointer */

		const char *data = 0;

		data = buffer_peek(buffer);
		TEST_ASSERT_EQUAL(data, 0);

		/* Write some */
		
		result = buffer_write(buffer, data1, data_len);
		TEST_ASSERT_EQUAL(result, (int32_t)data_len);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), data_len);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);

		/* Test that we can look at the data */

		data = buffer_peek(buffer);
		TEST_ASSERT_NOT_EQUAL(data, 0);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), data_len);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1024);
		for (i = 0; i < (int32_t)data_len; i++) {
			TEST_ASSERT_EQUAL(data1[i], data[i]);
		}

		/* Write more, of different lengths */

		result = buffer_write(buffer, data1, 512);
		TEST_ASSERT_EQUAL(result, 512);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), (3072 + 512));
		TEST_ASSERT_EQUAL(buffer_free(buffer), 512);

		result = buffer_write(buffer, data1, 512);
		TEST_ASSERT_EQUAL(result, 512);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 4096);
		TEST_ASSERT_EQUAL(buffer_used(buffer), (3072 + 1024));
		TEST_ASSERT_EQUAL(buffer_free(buffer), 0);

		result = buffer_write(buffer, data1, 1);
		TEST_ASSERT_EQUAL(result, 1);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 5120);
		TEST_ASSERT_EQUAL(buffer_used(buffer), (3072 + 1024 + 1));
		TEST_ASSERT_EQUAL(buffer_free(buffer), 1023);

		/* Test that the above writes can be read back */

		result = buffer_read(buffer, data2, 3072);
		TEST_ASSERT_EQUAL(result, 3072);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 5120);
		TEST_ASSERT_EQUAL(buffer_used(buffer), (1024 + 1));
		TEST_ASSERT_EQUAL(buffer_free(buffer), (1023 + 3072));
		for (i = 0; i < 3072; i++) {
			TEST_ASSERT_EQUAL(data1[i], data2[i]);
		}

		result = buffer_read(buffer, data2, 512);
		TEST_ASSERT_EQUAL(result, 512);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 5120);
		TEST_ASSERT_EQUAL(buffer_used(buffer), (512 + 1));
		TEST_ASSERT_EQUAL(buffer_free(buffer), (1535 + 3072));
		for (i = 0; i < 512; i++) {
			TEST_ASSERT_EQUAL(data1[i], data2[i]);
		}

		result = buffer_read(buffer, data2, 512);
		TEST_ASSERT_EQUAL(result, 512);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 5120);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 1);
		TEST_ASSERT_EQUAL(buffer_free(buffer), (5120 - 1));
		for (i = 0; i < 512; i++) {
			TEST_ASSERT_EQUAL(data1[i], data2[i]);
		}

		result = buffer_read(buffer, data2, 1);
		TEST_ASSERT_EQUAL(result, 1);
		TEST_ASSERT_EQUAL(buffer_size(buffer), 5120);
		TEST_ASSERT_EQUAL(buffer_used(buffer), 0);
		TEST_ASSERT_EQUAL(buffer_free(buffer), 5120);
		for (i = 0; i < 1; i++) {
			TEST_ASSERT_EQUAL(data1[i], data2[i]);
		}

		/* Test that read are getting the data in the right order */

		result = buffer_write(buffer, data1, 64);
		TEST_ASSERT_EQUAL(result, 64);

		result = buffer_write(buffer, data1 + 128, 64);
		TEST_ASSERT_EQUAL(result, 64);

		result = buffer_read(buffer, data2, 64);
		TEST_ASSERT_EQUAL(result, 64);
		for (i = 0; i < 64; i++) {
			TEST_ASSERT_EQUAL(data1[i], data2[i]);
		}

		result = buffer_write(buffer, data1 + 64, 64);
		TEST_ASSERT_EQUAL(result, 64);

		result = buffer_read(buffer, data2, 128);
		TEST_ASSERT_EQUAL(result, 128);
		for (i = 0; i < 128; i++) {
			if (i < 64) {
				TEST_ASSERT_EQUAL(data1[i+128], data2[i]);
			}
			else {
				TEST_ASSERT_EQUAL(data1[i], data2[i]);
			}
		}

		buffer_destroy(buffer);
	}
	free(data1);
	free(data2);
}
