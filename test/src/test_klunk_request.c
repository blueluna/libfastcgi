#include <stdint.h>
#include <assert.h>

#include "testcase.h"
#include "klunk_param.h"
#include "klunk_request.h"
#include "errorcodes.h"
#include "test_klunk_request.h"

void klunk_request_test()
{
	klunk_request_t* request = 0;

	request = klunk_request_create();
	TEST_ASSERT_NOT_EQUAL(request, 0);
	if (request != 0) {
		klunk_request_destroy(request);
	}
}
