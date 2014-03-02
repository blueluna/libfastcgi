#include <stdio.h>
#include <stdlib.h>

#include "test_llist.h"
#include "test_buffer.h"
#include "test_fcgi_param.h"
#include "test_fcgi_session.h"
#include "test_fcgi_context.h"

int main(void)
{
	llist_test();
	buffer_test();
	fcgi_param_test();
	fcgi_param_llist_test();
	fcgi_session_test();
	fcgi_context_test();
}
