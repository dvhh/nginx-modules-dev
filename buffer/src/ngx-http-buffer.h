#pragma once
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#ifndef _NGX_HTB_H_
#define _NGX_HTB_H_

/**
 * Configuration
 **/
typedef struct {
    /// enable buffer flag
    ngx_flag_t enabled;
} config_t;

/**
 * Request context
 **/
typedef struct {
    /// buffer chain storage
    ngx_chain_t *buffer_chain;
} context_t;

/**
 * Command 
 **/
static ngx_command_t  module_commands[] = {
    {
        /// enable buffer
        ngx_string("buffer_response"),
        NGX_HTTP_LOC_CONF | NGX_HTTP_LIF_CONF | NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(config_t, enabled),
        NULL
    },
    ngx_null_command
};

/**
 * Module initialization
 *
 * This will be used for rerouting the nginx filter routing
 **/
static ngx_int_t module_init(ngx_conf_t *config);

/**
 * Header filter
 *
 * This filter function is called after nginx has received and parsed the response headers from upstream
 **/
static ngx_int_t header_filter(ngx_http_request_t *request);

/**
 * Body filter
 *
 * This filter function is called each time after nginx has received a chunk of the response body
 **/
static ngx_int_t body_filter(ngx_http_request_t *request, ngx_chain_t *in);

/**
 * Allocate and initialize config
 **/
static void* create_conf(ngx_conf_t *config);

/**
 * Merge config
 */
static char *merge_conf(ngx_conf_t *config, void *parent, void *child);

/**
 * Send response header and body
 **/
static ngx_int_t send_response(ngx_http_request_t *request, const context_t *ctx);

static ngx_http_module_t  module_ctx = {
    NULL,           /* preconfiguration */
    module_init,    /* postconfiguration */

    NULL,           /* create main configuration */
    NULL,           /* init main configuration */

    NULL,           /* create server configuration */
    NULL,           /* merge server configuration */

    create_conf,    /* create location configuration */
    merge_conf      /* merge location configuration */
};

ngx_module_t  ngx_http_buffer_module = {
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
