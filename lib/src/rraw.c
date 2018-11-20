#include "rraw.h"
#include "../include/race.h"

rraw_t *rraw_new(void)
{
    rraw_t *rraw;

    if (!(rraw = malloc(sizeof(*rraw)))) {
        return (NULL);
    }
    (*rraw) = (rraw_t){
        .use_ssl = false,
        .uri = NULL,
    };
    TAILQ_INIT(&rraw->data);
    return (rraw);
}

int rraw_set_host(rraw_t *rraw, const char *uri, bool use_ssl)
{
    struct evhttp_uri *uri_new;

    if (!(uri_new = evhttp_uri_parse(uri))) {
        goto fail;
    }
    if (!evhttp_uri_get_host(uri_new) || !evhttp_uri_get_port(uri_new))
        goto fail;
    if (rraw->uri) {
        evhttp_uri_free(uri_new);
    }
    rraw->uri = uri_new;
    rraw->use_ssl = use_ssl;
    return (0);

fail:
    if (uri_new) {
        evhttp_uri_free(uri_new);
    }
    return (-1);
}

int rraw_set_race_param(rraw_t *rraw,
    unsigned count, const void *data, size_t len)
{
    if (!data) {
        return (-1);
    }
    for (unsigned i = 0; i < count; i++) {
        void *item_data;
        race_storage_item_t *item;

        if (!(item_data = malloc(len))) {
            return (-1);
        }
        memcpy(item_data, data, len);
        if (!(item = race_storage_item_new(item_data, len))) {
            free(item_data);
            return (-1);
        }
        TAILQ_INSERT_TAIL(&rraw->data, item, node);
    }
    return (0);
}

int rraw_set_race(race_t *r, rraw_t *rraw)
{
    race_conn_param_t param;
    race_storage_item_t *item;

    if (rraw->uri == NULL) {
        return (-1);
    }
    if (race_conn_param_init(&param, rraw->uri) < 0) {
        return (-1);
    }
    r->conn_param = (race_conn_param_t){
        .host = param.host,
        .port = param.port,
        .use_ssl = rraw->use_ssl,
    };
    TAILQ_INIT(&r->data);
    while (!TAILQ_EMPTY(&rraw->data)) {
        item = TAILQ_FIRST(&rraw->data);
        TAILQ_REMOVE(&rraw->data, item, node);
        TAILQ_INSERT_TAIL(&r->data, item, node);
    }
    evhttp_uri_free(rraw->uri);
    free(rraw);
    rraw = 0;
    return (0);
}
