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
#include "klunk_utils.h"
#include "buffer.h"

 struct my_server {
 	uv_handle_t		*server;
 	klunk_context_t	*ctx;
 };

 struct my_service_request {
 	uint16_t		id;
 	klunk_context_t *ctx;
 	buffer_t		*buffer;
 	uv_handle_t		*client;
 };

uv_loop_t *loop;

void signal_handler(uv_signal_t *handle, int signum)
{
    printf("signal received: %d\n", signum);
	struct my_server *server = (struct my_server*)loop->data;
	if (server != 0) {
    	klunk_destroy(server->ctx);
    	server->ctx = 0;
		uv_close(server->server, NULL);
    	free(server);
    	loop->data = 0;
	}
    uv_signal_stop(handle);
}

uv_buf_t alloc_buffer(uv_handle_t *handle, size_t suggested_size)
{
	return uv_buf_init((char*) malloc(suggested_size), suggested_size);
}

void client_destroy(uv_handle_t *handle)
{
	free(handle);
}

void print_params(llist_t *list)
{
	uint32_t p_count = 0;
	uint32_t name_len = 0;
	uint32_t value_len = 0;
	uint32_t name_len_max = 0;
	uint32_t value_len_max = 0;
	uint32_t name_len_acc = 0;
	uint32_t value_len_acc = 0;
	uint32_t name_value_len = 1024;
	char *name_value = malloc(name_value_len);
	uint32_t name_value_used = 0;
	uint32_t name_value_hash = 0;
	klunk_param_t *param = 0;
	llist_item_t *ptr = list->items;
	while (ptr != 0) {
		param = (klunk_param_t*)(ptr->data);
		name_len = strlen(param->name);
		value_len = strlen(param->value);
		name_value_used = name_len + value_len;
		if (name_value_used > name_value_len) {
			free(name_value);
			name_value = malloc(name_value_used);
		}
		memcpy(name_value, param->name, name_len);
		memcpy(name_value+name_len, param->value, value_len);
		name_value_hash = klunk_murmur3_32(name_value, name_value_used);
		printf("%08x: %s: %s\n", name_value_hash, param->name, param->value);
		name_len_max = name_len > name_len_max ? name_len : name_len_max;
		value_len_max = value_len > value_len_max ? value_len : value_len_max;
		name_len_acc += name_len;
		value_len_acc += value_len;
		p_count++;
		ptr = ptr->next;
	}
	
	printf("Count: %u\nName max length: %u\nValue max length: %u\n"
		"Name avg length: %.1f\nValue avg length: %.1f\n"
		, p_count, name_len_max, value_len_max
		, name_len_acc / (double)p_count, value_len_acc / (double)p_count);
	free(name_value);
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
		if (!klunk_param_is_free(param)) {
			result = snprintf(data, left, "<li><tt>%s</tt>: <tt>%s</tt></li>"
				, param->name, param->value);
			left = left - result;
			if (left <= 0) {
				break;
			}
		}
		data += result;
		ptr = ptr->next;
	}
}

void fcgi_write(uv_write_t *req, int status)
{
	int32_t result = 0;
	struct my_service_request *sr = 0;
	int32_t kstate = 0;

	sr = (struct my_service_request*)req->data;
		
	if (status == -1) {
		fprintf(stderr, "write error %s\n", uv_err_name(uv_last_error(loop)));
		if (uv_last_error(loop).code == UV_EPIPE) {
			uv_close(sr->client, client_destroy);
		}
	}
	else if (status == 0) {	
		kstate = klunk_request_state(sr->ctx, sr->id);

		if (kstate >= 0) {
			if ((kstate & KLUNK_RS_FINISH) == 0) {
				result = klunk_finish(sr->ctx, sr->id);
				assert(result == 0);
			}
			result = klunk_write(sr->ctx, buffer_peek(sr->buffer), buffer_free(sr->buffer), sr->id);
			// printf("fcgi_write: write content, %d\n", result);
			if (result > 0) {
				uv_buf_t b = { .len = result, .base = buffer_peek(sr->buffer) };
				uv_write(req, (uv_stream_t*)sr->client, &b, 1, fcgi_write);
				return;
			}
		}
	}
	/* close */
	// uv_close(sr->client, client_destroy);
	/* cleanup */
	buffer_destroy(sr->buffer);
	free(sr);
	free(req);
}

void fcgi_read(uv_stream_t *client, ssize_t nread, uv_buf_t buf)
{
	int32_t result = 0;
	klunk_context_t *ctx = 0;
	int32_t request_id = 0;
	int32_t kstate = 0;
	klunk_request_t* request = 0;

	// printf("fcgi_read: %lu\n", nread);

	if (nread == -1) {
		if (uv_last_error(loop).code != UV_EOF) {
			fprintf(stderr, "read error %s\n", uv_err_name(uv_last_error(loop)));
		}
		uv_close((uv_handle_t*)client, client_destroy);
	}
	else if (nread > 0) {
		if (loop->data != 0) {
			ctx = ((struct my_server*)loop->data)->ctx;
			result = klunk_read(ctx, buf.base, nread);
			result = klunk_current_request(ctx);
			if (result > 0) {
				request_id = result;
				kstate = klunk_request_state(ctx, request_id);
				request = klunk_find_request(ctx, request_id);
			}
			if ((kstate & KLUNK_RS_PARAMS_DONE)) {
				// printf("fcgi_read: params done\n");
				if (request != 0) {
					// print_params(request->params);
				}
			}
			if ((kstate & KLUNK_RS_STDIN_DONE)) {
				// printf("fcgi_read: stdin done\n");
				struct my_service_request *sr = (struct my_service_request*)malloc(sizeof(struct my_service_request));
				assert(sr != 0);

				sr->id = request_id;
				sr->ctx = ctx;
				sr->buffer = buffer_create();
				sr->client = (uv_handle_t*)client;
				
				uv_write_t *req = (uv_write_t*)malloc(sizeof(uv_write_t));
				assert(req != 0);
				req->data = sr;

				char *params = malloc(2048);
				assert(params != 0);
				if (request != 0) {
					format_params(request->params, params, 2048);
				}

				const char *null_content = "";

				const char *first = 
					"Status: 200\r\nContent-Type: text/html\r\n\r\n"
					"<html><head>"
					"<title>Klunk FastCGI Service</title>"
					"<style>html{font-family:sans-serif;background-color:#e5deca;color:#363d32;}</style>"
					"</head><body>"
					"<h1>Klunk FastCGI Service</h1>"
					"Network library: libuv</br>"
					"<h2>Request Parameters</h2>"
					"<ul>";
				result = klunk_write_output(ctx, request_id, first, strlen(first));
				result = klunk_write_output(ctx, request_id, params, strlen(params));
				const char *second = 
					"</ul>"
					"<h2>Request Content</h2>"
					"<pre>";
				result = klunk_write_output(ctx, request_id, second, strlen(second));

				const char *request_content = 0;
				if (buffer_used(request->content) > 0) {
					request_content = buffer_peek(request->content);
				}
				else {
					request_content = null_content;
				}

				result = klunk_write_output(ctx, request_id, request_content, strlen(request_content));
				const char *third = 
					"</pre></body></html>\r\n";
				result = klunk_write_output(ctx, request_id, third, strlen(third));

				free(params);
				// printf("fcgi_read: generate content, %d\n", content_len);

				assert(result > 0);
				// printf("fcgi_read: buffer content, %d\n", result);

				result = klunk_write(ctx, buffer_peek(sr->buffer), buffer_free(sr->buffer), request_id);
				assert(result > 0);
				// printf("fcgi_read: write content, %d\n", result);

				uv_buf_t b = { .len = result, .base = buffer_peek(sr->buffer) };

				uv_write(req, client, &b, 1, fcgi_write);
			}
		}
		else {
			uv_close((uv_handle_t*)client, client_destroy);
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
		uv_read_start((uv_stream_t*)client, alloc_buffer, fcgi_read);
	}
	else {
		uv_close((uv_handle_t*)client, client_destroy);
	}
}

int main()
{
	signal(SIGPIPE, SIG_IGN);

	loop = uv_default_loop();

	struct my_server *server_ctx = malloc(sizeof(struct my_server));
	assert(server_ctx != 0);

	// printf("create service\n");
	server_ctx->ctx = klunk_create();
	assert(server_ctx->ctx != 0);

	uv_signal_t sigint;
    uv_signal_init(loop, &sigint);
    uv_signal_start(&sigint, signal_handler, SIGINT);

	uv_tcp_t server;
	uv_tcp_init(loop, &server);

	struct sockaddr_in bind_addr = uv_ip4_addr("0.0.0.0", 8100);
	uv_tcp_bind(&server, bind_addr);
	uv_listen((uv_stream_t*)&server, 128, on_new_connection);

	server_ctx->server = (uv_handle_t*)&server;
	loop->data = server_ctx;

	uv_run(loop, UV_RUN_DEFAULT);

	return 0;
}
