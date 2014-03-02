#include <stdint.h>
#include <assert.h>

#include "testcase.h"
#include "fcgi_param.h"
#include "fcgi_session.h"
#include "errorcodes.h"
#include "test_fcgi_session.h"

void fcgi_session_test()
{
	fcgi_session_t* session = 0;

	session = fcgi_session_create();
	TEST_ASSERT_NOT_EQUAL(session, 0);
	if (session != 0) {
		fcgi_session_destroy(session);
	}
}
