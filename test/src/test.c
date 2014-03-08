#include <stdlib.h>

#include "test_llist.h"
#include "test_buffer.h"
#include "test_klunk_param.h"
#include "test_klunk_request.h"
#include "test_klunk_context.h"

int main(void)
{
	llist_test();
	buffer_test();
	klunk_param_test();
	klunk_param_llist_test();
	klunk_request_test();
	klunk_context_test();
}
