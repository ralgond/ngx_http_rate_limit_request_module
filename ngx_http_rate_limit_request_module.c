/*
 * Copyright (C) Teng Huang
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
    ngx_str_t uri;
} ngx_http_rate_limit_request_conf_t;

typedef struct {
    ngx_uint_t done;
    ngx_uint_t status;
    ngx_http_request_t *subrequest;
} ngx_http_rate_limit_request_ctx_t;

static ngx_int_t ngx_http_rate_limit_request_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_rate_limit_request_done(ngx_http_request_t *r,
    void *data, ngx_int_t rc);
static ngx_int_t ngx_http_rate_limit_request_init(ngx_conf_t *cf);
static char* ngx_http_rate_limit_request(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

static void *ngx_http_rate_limit_request_create_conf(ngx_conf_t *cf);
static char *ngx_http_rate_limit_request_merge_conf(ngx_conf_t *cf,
    void *parent, void *child);

static ngx_command_t ngx_http_rate_limit_request_commands[] = {
    {
        ngx_string("rate_limit_request"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_rate_limit_request,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },

    ngx_null_command
};

static ngx_http_module_t ngx_http_rate_limit_request_module_ctx = {
    NULL,
    ngx_http_rate_limit_request_init,

    NULL,
    NULL,

    NULL,
    NULL,

    ngx_http_rate_limit_request_create_conf,
    ngx_http_rate_limit_request_merge_conf
};

ngx_module_t ngx_http_rate_limit_request_module = {
    NGX_MODULE_V1,
    &ngx_http_rate_limit_request_module_ctx,
    ngx_http_rate_limit_request_commands,
    NGX_HTTP_MODULE,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_http_rate_limit_request_handler(ngx_http_request_t *r) {
    // ngx_table_elt_t *h, *ho, **ph;
    ngx_http_request_t *sr;
    ngx_http_post_subrequest_t *ps;
    ngx_http_rate_limit_request_ctx_t *ctx;
    ngx_http_rate_limit_request_conf_t *rlrcf;
    
    rlrcf = ngx_http_get_module_loc_conf(r, ngx_http_rate_limit_request_module);

    if (rlrcf->uri.len == 0) {
        return NGX_DECLINED;
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_rate_limit_request_module);

    if (ctx != NULL) {
        if (!ctx->done) {
            return NGX_AGAIN;
        }

        if (ctx->status == NGX_HTTP_FORBIDDEN || ctx->status == NGX_HTTP_TOO_MANY_REQUESTS) {
            return ctx->status;
        }

        if (ctx->status >= NGX_HTTP_OK
            && ctx->status < NGX_HTTP_SPECIAL_RESPONSE)
        {
            return NGX_OK;
        }

        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "rate limit request unexpected status: %ui", ctx->status);

        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_rate_limit_request_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ps = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
    if (ps == NULL) {
        return NGX_ERROR;
    }

    ps->handler = ngx_http_rate_limit_request_done;
    ps->data = ctx;

    if (ngx_http_subrequest(r, &rlrcf->uri, NULL, &sr, ps, NGX_HTTP_SUBREQUEST_WAITED) != NGX_OK) {
        return NGX_ERROR;
    }

    sr->request_body = ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));
    if (sr->request_body == NULL) {
        return NGX_ERROR;
    }

    sr->header_only = 1;

    ctx->subrequest = sr;

    ngx_http_set_ctx(r, ctx, ngx_http_rate_limit_request_module);

    return NGX_AGAIN;
}

static ngx_int_t ngx_http_rate_limit_request_done(ngx_http_request_t *r, void *data, ngx_int_t rc) {
    ngx_http_rate_limit_request_ctx_t *ctx = data;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "rate limit request done s:%ui", r->headers_out.status);

    ctx->done = 1;
    ctx->status = r->headers_out.status;

    return rc;
}

static void *ngx_http_rate_limit_request_create_conf(ngx_conf_t *cf) {
    ngx_http_rate_limit_request_conf_t *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_rate_limit_request_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    return conf;
}

static char *ngx_http_rate_limit_request_merge_conf(ngx_conf_t *cf, void *parent, void *child) {
    ngx_http_rate_limit_request_conf_t *prev = parent;
    ngx_http_rate_limit_request_conf_t *conf = child;

    ngx_conf_merge_str_value(conf->uri, prev->uri, "");

    return NGX_CONF_OK;
}

static ngx_int_t ngx_http_rate_limit_request_init(ngx_conf_t *cf) {
    ngx_http_handler_pt *h;
    ngx_http_core_main_conf_t *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_rate_limit_request_handler;

    return NGX_OK;
}

static char *ngx_http_rate_limit_request(ngx_conf_t *cf, ngx_command_t *cmd, void *conf) {
    ngx_http_rate_limit_request_conf_t* rlrcf = conf;

    ngx_str_t *value;

    if (rlrcf->uri.data != NULL) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        rlrcf->uri.len = 0;
        rlrcf->uri.data = (u_char*) "";
        return NGX_CONF_OK;
    }

    rlrcf->uri = value[1];

    return NGX_CONF_OK;
}