#include "ngx-http-buffer.h"
#include <stdbool.h>

#define BUFFERED 0x08

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
        ngx_log_error(
            NGX_LOG_WARN,
            request->connection->log,
            0,
            "disabled"
        );
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

static bool
add_buffer_piece(ngx_http_request_t *request, context_t *ctx, ngx_chain_t *in)
{
    for(ngx_chain_t *cl = in; cl != NULL; cl = cl->next) {
        buffer_piece_t *current = ngx_pcalloc(request->pool, sizeof(buffer_piece_t));
        if(current == NULL) {return false;}

        ngx_buf_t *b = cl->buf;
        if(b->in_file) {
            current->length = b->file_last - b->file_pos;
        }else{
            current->length = b->last - b->pos;
        }

        current->buffer = ngx_palloc(request->pool, current->length);
        if(current->buffer == NULL) {return false;}
        if(b->in_file) {
            ssize_t length = ngx_read_file(b->file, current->buffer, current->length, b->file_pos);
            if(length == -1) {return false;}
            if(length != current->length) {return false;}
        }else{
            unsigned char *end = ngx_cpymem(current->buffer, b->pos, current->length);
            if(end == NULL) {return false;}
            if((end - current->buffer) != current->length) {return false;}
        }
        if(ctx->last != NULL) {
            ctx->last->next = current;
        }
        ctx->last = current;
        if(ctx->head == NULL) {
            ctx->head = current;
        }
        if(b->last_buf) {
            return true;
        }
    } 
    return true;
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
        ngx_log_error(
            NGX_LOG_WARN,
            request->connection->log,
            0,
            "skip in"
        );
     
        request->connection->buffered |= NGX_HTTP_SSI_BUFFERED;
        request->buffered |= NGX_HTTP_SSI_BUFFERED;

        return ngx_http_next_body_filter(request, in);
        //return NGX_OK;
    }
    context_t *ctx = ngx_http_get_module_ctx(
            request,
            ngx_http_buffer_module
        );
    if(ctx == NULL) {
        ngx_log_error(
            NGX_LOG_WARN,
            request->connection->log,
            0,
            "skip ctx"
        );
        return ngx_http_next_body_filter(request, in);
    }
    if (!add_buffer_piece(request, ctx, in)) {
        ngx_log_error(
            NGX_LOG_ERR,
            request->connection->log,
            0,
            "could not copy buffer_chain"
    	);
	    return NGX_ERROR;
    }

    // check for last buffer
    bool last = false;
    size_t size = 0;
    ngx_chain_t *cl = in;
    while(cl != NULL) {
        ngx_buf_t *b = cl->buf;
        size_t current = 0;
        if(b->in_file) {
            current = b->file_last - b->file_pos;
            b->file_pos = b->file_last;
        }else{
            current = b->last - b->pos;
            //set buffer to zero length
            b->pos = b->last; 
            //b->last = b->pos;
        }
        size += current;
        //ngx_chain_t *old = cl;
        cl = cl->next;
        //ngx_free_chain(request->pool, old);

        if(b->last_buf) {
            last = true;
        }
    }
    size = 0;
    for(buffer_piece_t *i = ctx->head;i != NULL; i = i->next) {
        size += i->length;
    }
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "last = %i",
        last
    );
    if(last) {
        request->connection->buffered &= ~NGX_HTTP_SSI_BUFFERED;
        request->buffered &= ~NGX_HTTP_SSI_BUFFERED;
        return send_response(request, ctx);
    }
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "waiting remain of file %z",
        size
    );
    request->connection->buffered |= NGX_HTTP_SSI_BUFFERED;
    request->buffered |= NGX_HTTP_SSI_BUFFERED;

    return NGX_OK;
}


static
ngx_int_t send_response(ngx_http_request_t *request, const context_t *ctx)
{
    ngx_log_error(
        NGX_LOG_WARN,
        request->connection->log,
        0,
        "%s:%i:%s",
        __FILE__,
        __LINE__,
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
    for(buffer_piece_t *b = ctx->head; b != NULL; b = b->next) {
        size += b->length;
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
        "gathering buffers",
        size
    );
    for(buffer_piece_t *b = ctx->head; b != NULL; b = b->next) {
        buf->last = ngx_cpymem(buf->last, b->buffer, b->length);
        if(buf->last == NULL) {
            return NGX_ERROR;
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
