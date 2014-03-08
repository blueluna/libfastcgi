/* A FCGI service that use libuv
 *(C) 2014 Erik Svensson <erik.public@gmail.com>
 * Licensed under the MIT license.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <uv.h>
#include "klunk.h"
#include "buffer.h"

 struct service_request {
 	uint16_t		id;
 	klunk_context_t *ctx;
 	buffer_t		*buffer;
 };

uv_loop_t *loop;

uv_buf_t alloc_buffer(uv_handle_t *handle, size_t suggested_size)
{
	return uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void service_destroy(uv_handle_t *handle)
{
	if (handle->data != 0) {
		printf("destroy service\n");
		klunk_destroy(handle->data);
		handle->data = 0;
	}
	free(handle);
}

void print_params(llist_t *list)
{
	klunk_param_t *param = 0;
	llist_item_t *ptr = list->items;
	while (ptr != 0) {
		param = (klunk_param_t*)(ptr->data);
		printf("%s: %s\n", param->name, param->value);
		ptr = ptr->next;
	}
}

void format_params(llist_t *list, char *buffer, const int32_t len)
{
	char *data = buffer;
	int32_t left = len;
	int32_t result = 0;
	klunk_param_t *param = 0;
	llist_item_t *ptr = list->items;
	while (ptr != 0) {
		param = (klunk_param_t*)(ptr->data);
		result = snprintf(data, left, "<li><tt>%s</tt>: <tt>%s</tt></li>", param->name, param->value);
		left = left - result;
		if (left <= 0) {
			break;
		}
		data += result;
		ptr = ptr->next;
	}
}

void fcgi_write(uv_write_t *req, int status)
{
	int32_t result = 0;
	struct service_request *sr = 0;
	int32_t kstate = 0;

	if (status == -1) {
		fprintf(stderr, "write error %s\n", uv_err_name(uv_last_error(loop)));
	}
	else if (status == 0) {	
		sr = (struct service_request*)req->data;
		kstate = klunk_request_state(sr->ctx, sr->id);

		if (kstate >= 0) {
			if ((kstate & KLUNK_RS_FINISH) == 0) {
				result = klunk_finish(sr->ctx, sr->id);
				assert(result == 0);
			}
			result = klunk_write(sr->ctx, buffer_peek(sr->buffer), buffer_free(sr->buffer), sr->id);
			if (result > 0) {
				uv_buf_t b = { .len = result, .base = buffer_peek(sr->buffer) };
				uv_write(req, req->handle, &b, 1, fcgi_write);
				return;
			}
		}
		buffer_destroy(sr->buffer);
		free(sr);
		free(req);
	}
}

void fcgi_read(uv_stream_t *client, ssize_t nread, uv_buf_t buf)
{
	int32_t result = 0;
	klunk_context_t *ctx = 0;
	int32_t request_id = 0;
	int32_t kstate = 0;
	klunk_request_t* request = 0;


	if (nread == -1) {
		if (uv_last_error(loop).code != UV_EOF) {
			fprintf(stderr, "read error %s\n", uv_err_name(uv_last_error(loop)));
		}
		uv_close((uv_handle_t*)client, service_destroy);
	}
	else if (nread > 0) {
		if (client->data != 0) {
			ctx = (klunk_context_t*)client->data;
			result = klunk_read(ctx, buf.base, nread);
			result = klunk_current_request(ctx);
			if (result > 0) {
				request_id = result;
				kstate = klunk_request_state(ctx, request_id);
				request = klunk_find_request(ctx, request_id);
			}
			if ((kstate & KLUNK_RS_PARAMS_DONE)) {
				if (request != 0) {
					print_params(request->params);
				}
			}
			if ((kstate & KLUNK_RS_STDIN_DONE)) {
				struct service_request *sr = (struct service_request*)malloc(sizeof(struct service_request));
				assert(sr != 0);

				sr->id = request_id;
				sr->ctx = ctx;
				sr->buffer = buffer_create();
				
				uv_write_t *req = (uv_write_t*)malloc(sizeof(uv_write_t));
				assert(req != 0);
				req->data = sr;

				char *params = malloc(2048);
				assert(params != 0);
				if (request != 0) {
					format_params(request->params, params, 2048);
				}
				char *content = malloc(3072);
				assert(params != 0);

				const char *null_content = "";

				const char *fmt = 
					"Status: 200\r\nContent-Type: text/html\r\n\r\n"
					"<html><head>"
					"<title>Klunk FastCGI Service</title>"
					"<style>html{font-family:sans-serif;background-color:#e5deca;color:#363d32;}</style>"
					"</head><body>"
					"<h1>Klunk FastCGI Service</h1>"
					"FastCGI id: %d</br>"
					"Network library: libuv</br>"
					"<h2>Request Parameters</h2>"
					"<ul>%s</ul>"
					"<h2>Request Content</h2>"
					"<pre>%s</pre>"
					"</body></html>\r\n";

				const char *request_content = 0;
				if (buffer_used(request->content) > 0) {
					request_content = buffer_peek(request->content);
				}
				else {
					request_content = null_content;
				}
				int content_len = snprintf(content, 3072, fmt, request_id, params, request_content);

				free(params);

				result = klunk_write_output(ctx, request_id, content
					, content_len);
				assert(result > 0);

				free(content);

				result = klunk_write(ctx, buffer_peek(sr->buffer), buffer_free(sr->buffer), request_id);
				assert(result > 0);

				uv_buf_t b = { .len = result, .base = buffer_peek(sr->buffer) };

				uv_write(req, client, &b, 1, fcgi_write);
			}
		}
		else {
			uv_close((uv_handle_t*)client, service_destroy);
		}
	}
	free(buf.base);
}

void on_new_connection(uv_stream_t *server, int status)
{
	if (status == -1) {
		// error!
		return;
	}

	uv_tcp_t *client = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	assert(client != 0);
	uv_tcp_init(loop, client);
	int r = uv_accept(server, (uv_stream_t*)client);
	if (r == 0) {
		printf("create service\n");
		client->data = klunk_create();
		assert(client->data != 0);
		uv_read_start((uv_stream_t*)client, alloc_buffer, fcgi_read);
	}
	else {
		uv_close((uv_handle_t*)client, NULL);
	}
}

int main()
{
	loop = uv_default_loop();

	uv_tcp_t server;
	uv_tcp_init(loop, &server);

	struct sockaddr_in bind_addr = uv_ip4_addr("0.0.0.0", 8100);
	uv_tcp_bind(&server, bind_addr);
	uv_listen((uv_stream_t*)&server, 128, on_new_connection);

	uv_run(loop, UV_RUN_DEFAULT);

	return 0;
}
