#include "errorcodes.h"
#include "klunk_request.h"
#include "fcgi_param.h"

#include <stdlib.h>

klunk_request_t* klunk_request_create()
{
	klunk_request_t *session = 0;
	session = malloc(sizeof(klunk_request_t));
	if (session != 0) {
		session->id = 0;
		session->role = 0;
		session->flags = 0;
		session->state = 0;
		session->params = 0;
		session->content = 0;

		session->params = llist_create(sizeof(fcgi_param_t));
		if (session->params == 0) {
			free(session);
			session = 0;
		}
		else {
			llist_register_dtor(session->params
				, (llist_item_dtor)&fcgi_param_destroy);
		}
	}
	if (session != 0) {
		session->content = buffer_create();
		if (session->content == 0) {
			llist_destroy(session->params);
			session->params = 0;
			free(session);
			session = 0;
		}
	}
	return session;
}

void klunk_request_destroy(klunk_request_t *session)
{
	if (session != 0) {
		buffer_destroy(session->content);
		session->content = 0;
		llist_destroy(session->params);
		session->params = 0;
		free(session);
	}
}
