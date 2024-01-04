#include "ngx-http-passthru.h"

static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;

static ngx_int_t
module_init(ngx_conf_t *config)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = body_filter;

    return NGX_OK;
}

static ngx_int_t
header_filter(ngx_http_request_t *request)
{
	return ngx_http_next_header_filter(request);
}

static ngx_int_t
body_filter(ngx_http_request_t *request, ngx_chain_t *in)
{
	return ngx_http_next_body_filter(request, in);
}