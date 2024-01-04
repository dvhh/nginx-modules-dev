#pragma once
#include <ngx_core.h>
#include <ngx_http.h>

#ifndef _NGX_HTPT_H_
#define _NGX_HTPT_H_

static ngx_command_t  module_commands[] = {
    ngx_null_command
};

static ngx_int_t module_init(ngx_conf_t *config);
static ngx_int_t header_filter(ngx_http_request_t *request);
static ngx_int_t body_filter(ngx_http_request_t *request, ngx_chain_t *in);

static ngx_http_module_t  module_ctx = {
    NULL,           /* preconfiguration */
    module_init,    /* postconfiguration */

    NULL,           /* create main configuration */
    NULL,           /* init main configuration */

    NULL,           /* create server configuration */
    NULL,           /* merge server configuration */

    NULL,           /* create location configuration */
    NULL            /* merge location configuration */
};

ngx_module_t  ngx_http_passthru_module = {
    NGX_MODULE_V1,
    &module_ctx,        /* module context */
    module_commands,    /* module directives */
    NGX_HTTP_MODULE,    /* module type */
    NULL,               /* init master */
    NULL,               /* init module */
    NULL,               /* init process */
    NULL,               /* init thread */
    NULL,               /* exit thread */
    NULL,               /* exit process */
    NULL,               /* exit master */
    NGX_MODULE_V1_PADDING
};

#endif // _NGX_HTB_H_
