#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>
#include <ngx_stream_probe.h>
static ngx_int_t ngx_stream_probe_init(ngx_conf_t *cf);
static void * ngx_stream_probe_create_srv_conf(ngx_conf_t *cf);
static char * ngx_stream_probe_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_stream_probe_init(ngx_conf_t *cf);
typedef struct {
  ngx_flag_t                     probe_enable;
  ngx_msec_t                     probe_timeout;
} ngx_stream_probe_srv_conf_t;

static ngx_command_t  ngx_stream_probe_commands[] = {
  { ngx_string("probe_enable"),
    NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
    ngx_conf_set_flag_slot,
    NGX_STREAM_SRV_CONF_OFFSET,
    offsetof(ngx_stream_probe_srv_conf_t, probe_enable),
    NULL },
  { ngx_string("probe_timeout"),
      NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_msec_slot,
      NGX_STREAM_SRV_CONF_OFFSET,
      offsetof(ngx_stream_probe_srv_conf_t, probe_timeout),
      NULL },
  ngx_null_command
};

static ngx_stream_module_t  ngx_stream_probe_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_stream_probe_init,                /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    ngx_stream_probe_create_srv_conf,     /* create server configuration */
    ngx_stream_probe_merge_srv_conf       /* merge server configuration */
};

ngx_module_t  ngx_stream_probe_module = {
    NGX_MODULE_V1,
    &ngx_stream_probe_module_ctx,          /* module context */
    ngx_stream_probe_commands,             /* module directives */
    NGX_STREAM_MODULE,                     /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_int_t
ngx_stream_probe_handler(ngx_stream_session_t *s)
{
  ngx_stream_probe_srv_conf_t  *pscf;
  ngx_connection_t             *c;
  c = s->connection;
  pscf = ngx_stream_get_module_srv_conf(s, ngx_stream_probe_module);
  if (c->read->timedout) {
    ngx_log_error(NGX_LOG_INFO, c->log, 0, "[probe][%d]recyle stream session:%d", c->fd, s->signature);
    ngx_stream_finalize_session(s, NGX_STREAM_OK);
    c->timedout = 1;
    return NGX_DONE;
  } else if (pscf->probe_enable && ngx_process_probe(c) == NGX_OK) {
    // ngx_stream_proxy_finalize(s, NGX_STREAM_OK);
    if (c->read != NULL) {
      ngx_add_timer(c->read, pscf->probe_timeout);
    }
    return NGX_DONE;
  }
  return NGX_OK;
}

static ngx_int_t ngx_stream_probe_init(ngx_conf_t *cf)
{
  ngx_stream_handler_pt        *h;
  ngx_stream_core_main_conf_t  *cmcf;

  cmcf = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_core_module);

  h = ngx_array_push(&cmcf->phases[NGX_STREAM_PROBE_PHASE].handlers);
  if (h == NULL) {
      return NGX_ERROR;
  }

  *h = ngx_stream_probe_handler;
  return NGX_OK;
}

static void *
ngx_stream_probe_create_srv_conf(ngx_conf_t *cf)
{
    ngx_stream_probe_srv_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_stream_probe_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    conf->probe_enable = NGX_CONF_UNSET;
    conf->probe_timeout = NGX_CONF_UNSET_MSEC;
    return conf;
}


static char *
ngx_stream_probe_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_stream_probe_srv_conf_t  *prev = parent;
    ngx_stream_probe_srv_conf_t  *conf = child;
    ngx_conf_merge_value(conf->probe_enable, prev->probe_enable, 0);
    ngx_conf_merge_msec_value(conf->probe_timeout,
                              prev->probe_timeout, 30000);
    return NGX_CONF_OK;
}