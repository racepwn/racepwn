#include "race_int.h"
// Fix error with strcasecmp
#include <strings.h>

race_t *race_new(void)
{
    race_t *r;

    if (!(r = calloc(1, sizeof(*r)))) {
        goto fail;
    }
    if (!(r->storage = race_storage_new())) {
        goto fail;
    }
    return r;

fail:
    if (r) {
        race_free(r);
    }
    return NULL;
}

void race_free(race_t *r)
{
    if (r->storage) {
        race_storage_free(r->storage);
    }
    if (r->conn_param) {
        race_conn_param_free(r->conn_param);
    }
    if (r->err) {
        race_err_free(r->err);
    }
    free(r);
    r = NULL;
}

race_status_t race_set_option(race_t *r, race_options_t opt, size_t option)
{
    unsigned long ulong_opt = (unsigned long)option;
    bool bool_opt = option ? true : false;
    char *char_opt = (char *)option;
    switch (opt) {
    case RACE_OPT_MODE_PARALELLS:
        if (bool_opt) {
            r->race_param.mode = RACE_PARALELLS;
        }
        break;
    case RACE_OPT_MODE_PIPELINE:
        if (bool_opt) {
            r->race_param.mode = RACE_PIPELINE;
        }
        break;
    case RACE_OPT_URL:
        return race_set_url(r, char_opt, r->cached_opt.use_ssl);
    case RACE_OPT_USE_SSL:
        r->cached_opt.use_ssl = bool_opt;
        if (r->conn_param) {
            r->conn_param->use_ssl = bool_opt;
        }
        break;
    case RACE_OPT_LAST_CHUNK_DELAY_USEC:
        r->race_param.delay = (struct timeval){
            .tv_sec = ulong_opt / 1000000,
            .tv_usec = ulong_opt % 1000000,
        };
        break;
    case RACE_OPT_LAST_CHUNK_SIZE:
        r->race_param.last_chunk_size = ulong_opt;
        break;
    default:
        r->err = race_err_print(RACE_BAD_ARGS, "race set option: unknown option");
        return r->err->status;
    }
    return RACE_OK;
}

race_status_t race_run(race_t *r, race_info_t **info)
{
    race_job_t *job = NULL;
    race_info_t *r_info = NULL;

    if (r->err) {
        race_err_t *err = race_err_print(
            RACE_ERROR, "race run: race have error: %s", race_strerror(r));
        race_err_free(r->err);
        r->err = err;
        goto fail;
    }
    if (!r->conn_param) {
        r->err = race_err_print(RACE_BAD_ARGS,
            "race run: host and port must be specified");
        goto fail;
    }
    if (!r->storage->count) {
        r->err = race_err_print(RACE_BAD_ARGS, "race run: no data to send");
        goto fail;
    }
    if (r->race_param.mode == RACE_PIPELINE) {
        race_buf_t *buf_merged = NULL;
        while (!STAILQ_EMPTY(r->storage)) {
            race_buf_t *buf;
            race_buf_t *buf_storage = STAILQ_FIRST(r->storage);
            STAILQ_REMOVE(r->storage, buf_storage, race_buf_s, node);
            if (!(buf = race_buf_merge(buf_merged, buf_storage))) {
                r->err = race_err_base(RACE_ERR_MEMORY);
                race_buf_free(buf_merged);
                goto fail;
            }
            buf_merged = buf;
            r->storage->count--;
        }
        race_storage_insert(r->storage, buf_merged);
        r->race_param.last_chunk_size = 0;
        r->race_param.delay = (struct timeval){ 0, 0 };
    }
    if (!(r_info = race_info_new(r->storage->count))) {
        goto fail;
    }
    if (!(job = race_job_new(r, r_info))) {
        goto fail;
    }
    if (race_job_run(job) != RACE_OK) {
        goto fail;
    }
    if (info != NULL) {
        (*info) = r_info;
    } else {
        race_info_free(r_info);
    }
    race_job_free(job);
    return RACE_OK;

fail:
    (*info) = NULL;
    if (r_info) {
        race_info_free(r_info);
    }
    if (job) {
        race_job_free(job);
    }
    return r->err->status;
}

const char *race_strerror(race_t *r)
{
    if (!r->err) {
        return NULL;
    }
    return r->err->str;
}

race_status_t race_set_url(race_t *r, const char *url, bool use_ssl)
{
    race_conn_param_t *param = NULL;
    struct evhttp_uri *uri = NULL;
    const char *host;
    const char *scheme;
    int scheme_port = 0;
    int port;

    if (r->err) {
        return RACE_ERROR;
    }
    if (!(uri = evhttp_uri_parse(url))) {
        r->err = race_err_print(RACE_BAD_ARGS, "uri %s: parse error", url);
        goto fail;
    }
    if (!(scheme = evhttp_uri_get_scheme(uri))) {
        r->err = race_err_print(RACE_BAD_ARGS, "uri %s: scheme must be specified", url);
        goto fail;
    }
    if (!strcasecmp(scheme, "http")) {
        scheme_port = 80;
    } else if (!strcasecmp(scheme, "https")) {
        scheme_port = 443;
    } else if (!strcasecmp(scheme, "tcp")) {
        scheme_port = 0;
    } else {
        r->err = race_err_print(RACE_BAD_ARGS,
            "uri %s: supported schemes: tcp, http, https", url);
        goto fail;
    }
    if (!(host = evhttp_uri_get_host(uri))) {
        r->err = race_err_print(RACE_BAD_ARGS, "uri %s: host must be specified", url);
        goto fail;
    }
    if ((port = evhttp_uri_get_port(uri)) == -1) {
        if (!scheme_port) {
            r->err = race_err_print(RACE_BAD_ARGS, "uri %s: port must be specified", url);
            goto fail;
        }
        port = scheme_port;
    }
    if ((r->err = race_conn_param_init(&param, host, port, use_ssl))) {
        goto fail;
    }
    if (r->conn_param) {
        race_conn_param_free(r->conn_param);
    }
    r->conn_param = param;
    evhttp_uri_free(uri);
    return RACE_OK;

fail:
    if (uri) {
        evhttp_uri_free(uri);
    }
    return r->err->status;
}

race_info_t *race_info_new(unsigned count)
{
    race_info_t *info;

    if (!(info = calloc(1, sizeof(*info)))) {
        goto fail;
    }
    if (!(info->conn_statuses = calloc(count, sizeof(*info->conn_statuses)))) {
        goto fail;
    }
    info->count = count;
    return info;

fail:
    if (info) {
        race_info_free(info);
    }
    return NULL;
}

void race_info_free(race_info_t *info)
{
    if (!info) {
        return;
    }
    if (info->conn_statuses) {
        for (unsigned int i = 0; i < info->count; i++) {
            race_conn_status_t *conn_status = &info->conn_statuses[i];
            if (conn_status->buf) {
                race_buf_free(conn_status->buf);
            }
        }
        free(info->conn_statuses);
    }
    if (info->global_status) {
        free(info->global_status);
    }
    free(info);
    info = NULL;
}

const char *race_info_get_status(race_info_t *info)
{
    if (!info->global_status) {
        return "";
    }
    return info->global_status;
}

unsigned race_info_count(race_info_t *info)
{
    return info->count;
}

race_response_t race_info_get_response(race_info_t *info, unsigned idx)
{
    race_buf_t *buf = info->conn_statuses[idx].buf;
    if (!buf) {
        return (race_response_t){ .data = "", .len = 1 };
    }
    return (race_response_t){ .data = buf->data, .len = buf->len };
}
