#include "ngx-http-buffer.h"
#include <stdbool.h>

static void *
create_conf(ngx_conf_t *config)
{
    ngx_log_error(
        NGX_LOG_WARN,
        config->log,
        0,
        __PRETTY_FUNCTION__
    );
    config_t *result;
    result = ngx_pcalloc(config->pool, sizeof(config_t));
    if(result == NULL) {return NULL;}
    result->enabled = NGX_CONF_UNSET;

    return result;
}

static char*
merge_conf(ngx_conf_t *config, void* parent, void* child)
{
	const config_t *prev = parent;
	config_t *conf = child;

	ngx_conf_merge_value(conf->enabled, prev->enabled, 0);

	return NGX_CONF_OK;
}

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static ngx_int_t
module_init(ngx_conf_t *config)
{
    ngx_log_error(
        NGX_LOG_WARN,
        config->log,
        0,
        __PRETTY_FUNCTION__
    );
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = body_filter;

    return NGX_OK;
}

static ngx_int_t
header_filter(ngx_http_request_t *request)
{
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        __PRETTY_FUNCTION__
    );
    const config_t *config = ngx_http_get_module_loc_conf(
        request, ngx_http_buffer_module
    );
    if(config == NULL) {return NGX_ERROR;}
    if(! config->enabled) {
        return ngx_http_next_header_filter(request);
    }

    context_t *ctx = ngx_pcalloc(request->pool, sizeof(context_t));

    if(ctx == NULL) {
            ngx_log_error(
                NGX_LOG_ERR,
                request->connection->log,
                0,
                "could not allocate context"
            );
            return NGX_ERROR;
    }
    ngx_http_set_ctx(request, ctx, ngx_http_buffer_module);
    return NGX_OK;
}

static ngx_int_t
body_filter(ngx_http_request_t *request, ngx_chain_t *in)
{
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        __PRETTY_FUNCTION__
    );
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "%p %p",
        request,
        in
    );
    
    if(in == NULL) {
        return ngx_http_next_body_filter(request, in);
        //return NGX_OK;
    }
    context_t *ctx = ngx_http_get_module_ctx(
            request,
            ngx_http_buffer_module
        );
    if(ctx == NULL) {
        return ngx_http_next_body_filter(request, in);
    }
    if (ngx_chain_add_copy(request->pool, &(ctx->buffer_chain), in) != NGX_OK) {
        ngx_log_error(
            NGX_LOG_ERR,
            request->connection->log,
            0,
            "could not copy buffer chain"
        );
        return NGX_ERROR;
    };
    // check for last buffer
    ssize_t size = 0;
    for(ngx_chain_t *cl = ctx->buffer_chain; cl != NULL; cl = cl->next) {
        ngx_buf_t *b = cl->buf;
        size_t current = 0;
        if(b->in_file) {
            current = b->file_last - b->file_pos;
        }else{
            current = b->last - b->pos;
        }
        size += current;
        if(b->last_buf) {
            return send_response(request, ctx);
        }
    }
    if(request->headers_out.content_length_n != -1) {
        if(size == request->headers_out.content_length_n) {
            return send_response(request, ctx);
        }
    }

    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "waiting remain of file %z",
        size
    );
    request->connection->buffered |= BUFFERED;
    //return NGX_AGAIN;
    return NGX_OK;
}


static
ngx_int_t send_response(ngx_http_request_t *request, const context_t *ctx)
{
    request->connection->buffered &= ~BUFFERED;
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        __PRETTY_FUNCTION__
    );
    ngx_int_t result = ngx_http_next_header_filter(request);
    if(result == NGX_ERROR) {
        return NGX_ERROR;
    }
    if(request->header_only) {
        return NGX_OK;
    }
    size_t size = 0;
    for(ngx_chain_t *cl = ctx->buffer_chain; cl != NULL; cl = cl->next) {
        ngx_buf_t *b = cl->buf;
        size_t current = 0;
        if(b->in_file) {
            current = b->file_last - b->file_pos;
        }else{
            current = b->last - b->pos;
        }
        size += current;
    }
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "buffer size %z",
        size
    );
    ngx_buf_t *buf = ngx_pcalloc(request->pool, sizeof(ngx_buf_t));
    //ngx_memzero(&buf, sizeof(buf));
    buf->pos      = ngx_palloc(request->pool, size);
    if(buf->pos == NULL){
        return NGX_ERROR;
    }
    buf->start         = buf->pos;
    buf->end           = buf->pos + size;
    buf->last          = buf->pos;
    buf->memory        = true;
    buf->last_buf      = true;

    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "gathering chains",
        size
    );
    for(ngx_chain_t *cl = ctx->buffer_chain; cl != NULL; cl = cl->next) {
        ngx_buf_t *b = cl->buf;
        if(b->in_file) {
            ssize_t read_bytes =ngx_read_file(b->file, buf->last, b->file_last - b->file_pos, b->file_pos);
            if(read_bytes == -1) {
                return NGX_ERROR;
            }
            buf->last += read_bytes;
        }else{
            buf->last = ngx_cpymem(buf->last, b->pos, b->last - b->pos);
            if(buf->last == NULL) {
                return NGX_ERROR;
            }
        }
        if(b->last_buf) {
            break;
        }
    }
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "done %z / %z",
        buf->last - buf->pos,
        size
    );

    ngx_chain_t out;
    out.buf  = buf;
    out.next = NULL;

    //return NGX_OK;
    return ngx_http_next_body_filter(request, &out);
}
