#include "race_module.h"

static void
race_timeout_cb(evutil_socket_t fd, short what, void *_ctx)
{
    race_arg_t *arg = _ctx;
    race_ctx_t *ctx = arg->ctx;
    for (unsigned i = 0; i < ctx->event_ctx_count; i++) {
        race_storage_item_t *wr_storage = arg->write_storage;
        race_event_ctx_t *ev_ctx = ctx->event_ctx[i];
        if (!ev_ctx) {
            continue;
        }
        bufferevent_write(ev_ctx->bev, wr_storage->data, wr_storage->len);
        bufferevent_enable(ev_ctx->bev, EV_READ | EV_WRITE);
    }
}

static void
race_timer_init(race_arg_t *arg)
{
    race_ctx_t *ctx = arg->ctx;
    struct event *timer_ev = evtimer_new(ctx->base, race_timeout_cb, arg);
    if (!timer_ev) {
        event_base_loopexit(ctx->base, NULL);
        return;
    }
    evtimer_add(timer_ev, &ctx->r->race_param.delay);
    ctx->timer_ev = timer_ev;
}

static void
race_read_cb(struct bufferevent *bev, void *ctx)
{
    return;
}

static void
race_write_cb(struct bufferevent *bev, void *ctx)
{
    race_arg_t *arg = ctx;
    race_ctx_t *race_ctx = arg->ctx;

    race_ctx->buffers_left--;
    if (!race_ctx->buffers_left) {
        switch (race_ctx->state) {
        case RACE_WAIT_TIMEOUT:
        case RACE_SEND_CONNECT:
            break;
        case RACE_SEND_DATA:
            if (race_ctx->r->race_param.last_chunk_size != 0) {
                race_timer_init(arg);
                race_ctx->state = RACE_SEND_LAST_CHUNK;
            } else {
                event_base_loopexit(race_ctx->base, NULL);
            }
            break;
        case RACE_SEND_LAST_CHUNK:
            event_base_loopexit(race_ctx->base, NULL);
            break;
        }
    }
}

static void
race_event_cb(struct bufferevent *bev, short what, void *ctx)
{
    race_arg_t *arg = ctx;
    race_ctx_t *race_ctx = arg->ctx;

    switch (what) {
    case BEV_EVENT_CONNECTED:
        race_ctx->buffers_connected++;
        break;
    default:
        if (race_ctx->state == RACE_SEND_CONNECT) {
            race_ctx->buffers_err++;
        }
        race_ctx->buffers_left--;
        break;
    }
    if (!race_ctx->buffers_left) {
        switch (race_ctx->state) {
        case RACE_SEND_LAST_CHUNK:
        case RACE_SEND_CONNECT:
        case RACE_WAIT_TIMEOUT:
            event_base_loopexit(race_ctx->base, NULL);
            break;
        case RACE_SEND_DATA:
            if (race_ctx->r->race_param.last_chunk_size != 0) {
                race_timer_init(arg);
            }
            break;
        }
    }
    if (race_ctx->state == RACE_SEND_CONNECT) {
        unsigned buf_total = race_ctx->buffers_err + race_ctx->buffers_left;
        if (buf_total == race_ctx->event_ctx_count) {
            race_ctx->state = RACE_SEND_DATA;
        }
    }
}

static int
race_base_init(void **_ctx, race_t *r, struct event_base *base)
{
    race_storage_item_t *item;
    race_ctx_t *ctx;
    race_event_ctx_t **event_ctx;
    unsigned event_ctx_count;

    if (!(ctx = malloc(sizeof(*ctx)))) {
        return (-1);
    }
    event_ctx_count = 0;
    TAILQ_FOREACH(item, &r->data, node)
    {
        event_ctx_count++;
    }
    if (!(event_ctx = calloc(event_ctx_count, sizeof(*event_ctx)))) {
        goto fail;
    }
    (*ctx) = (race_ctx_t){
        .state = RACE_SEND_CONNECT,
        .r = r,
        .base = base,
        .event_ctx = event_ctx,
        .event_ctx_count = event_ctx_count,
        .timer_ev = NULL,
        .buffers_left = 0,
        .buffers_connected = 0,
        .buffers_err = 0,
    };
    unsigned i = 0;
    TAILQ_FOREACH(item, &r->data, node)
    {
        race_event_ctx_t *ev_ctx;
        ev_ctx = race_event_ctx_init(ctx, item);
        if (!ev_ctx) {
            break;
        }
        event_ctx[i] = ev_ctx;
        ctx->buffers_left++;
        i++;
    }
    (*_ctx) = ctx;
    return (0);

fail:
    if (event_ctx) {
        for (unsigned i = 0; i < event_ctx_count; i++) {
            race_event_ctx_t *ev_ctx = event_ctx[i];
            if (!ev_ctx) {
                continue;
            }
            race_event_ctx_free(ev_ctx);
        }
        free(event_ctx);
    }
    free(ctx);
    return (-1);
}

static void
race_base_deinit(void *_ctx)
{
    race_ctx_t *ctx = _ctx;
    for (unsigned i = 0; i < ctx->event_ctx_count; i++) {
        race_event_ctx_t *ev_ctx = ctx->event_ctx[i];
        if (!ev_ctx) {
            continue;
        }
        race_event_ctx_free(ev_ctx);
    }
    if (ctx->timer_ev) {
        event_free(ctx->timer_ev);
    }
    free(ctx->event_ctx);
    free(ctx);
    ctx = NULL;
}

void race_module_paraleels_init(race_module_t *m)
{
    (*m) = (race_module_t){
        .ctx = NULL,
        .init = race_base_init,
        .deinit = race_base_deinit,
    };
}

static int
race_pipeline_init(void **_ctx, race_t *r, struct event_base *base)
{
    void *all_in_one_data, *ptr;
    size_t total_len = 0;
    race_storage_item_t *item;
    r->race_param.last_chunk_size = 0;
    TAILQ_FOREACH(item, &r->data, node)
    {
        total_len += item->len;
    }
    all_in_one_data = malloc(total_len);
    if (!all_in_one_data) {
        return (-1);
    }
    ptr = all_in_one_data;
    while (!TAILQ_EMPTY(&r->data)) {
        race_storage_item_t *item = TAILQ_FIRST(&r->data);
        TAILQ_REMOVE(&r->data, item, node);
        memcpy(ptr, item->data, item->len);
        ptr += item->len;
        race_storage_item_free(item);
    }
    item = race_storage_item_new(all_in_one_data, total_len);
    if (!item) {
        free(all_in_one_data);
        return (-1);
    }
    TAILQ_INSERT_HEAD(&r->data, item, node);
    return (race_base_init(_ctx, r, base));
}

void race_module_pipeline_init(race_module_t *m)
{
    (*m) = (race_module_t){
        .ctx = NULL,
        .init = race_pipeline_init,
        .deinit = race_base_deinit,
    };
}

race_event_ctx_t *
race_event_ctx_init(race_ctx_t *race_ctx, race_storage_item_t *write_storage)
{
    race_conn_param_t *conn = &race_ctx->r->conn_param;
    race_event_ctx_t *event_ctx;
    struct bufferevent *bev;
    SSL_CTX *ssl_ctx = NULL;
    struct evutil_addrinfo *answer = NULL;
    SSL *ssl;
    evutil_socket_t sock = 0;

    if (!(event_ctx = malloc(sizeof(*event_ctx)))) {
        return (NULL);
    }
    if ((sock = race_tcp_sock_create(conn->host, conn->port, &answer)) < 0) {
        goto fail;
    }
    if (conn->use_ssl) {
        if (!(ssl_ctx = race_ssl_ctx_new())) {
            close(sock);
            goto fail;
        }
        ssl = SSL_new(ssl_ctx);
        // bev = bufferevent_openssl_socket_new(race_ctx->base,
        // sock, ssl, BUFFEREVENT_SSL_OPEN, BEV_OPT_CLOSE_ON_FREE);
    } else {
        bev = bufferevent_socket_new(race_ctx->base, sock, BEV_OPT_CLOSE_ON_FREE);
    }
    if (!bev) {
        goto fail;
    }
    bufferevent_setcb(bev, race_read_cb, race_write_cb,
        race_event_cb, &event_ctx->arg);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    if (race_ctx->r->race_param.last_chunk_size) {
        size_t left = race_ctx->r->race_param.last_chunk_size;
        bufferevent_write(bev, write_storage->data, write_storage->len - left);
        race_storage_realloc_left(write_storage, left);
    } else {
        bufferevent_write(bev, write_storage->data, write_storage->len);
        free(write_storage->data);
        write_storage->data = NULL;
    }
    bufferevent_socket_connect(bev, answer->ai_addr, answer->ai_addrlen);
    evutil_freeaddrinfo(answer);
    (*event_ctx) = (race_event_ctx_t){
        .arg = (race_arg_t){
            .ctx = race_ctx,
            .write_storage = write_storage,
        },
        .ssl = ssl,
        .ssl_ctx = ssl_ctx,
        .bev = bev,
    };
    return (event_ctx);

fail:
    if (ssl_ctx) {
        race_ssl_ctx_free(ssl_ctx);
        SSL_free(ssl);
    }
    if (sock > 0) {
        close(sock);
    }
    free(event_ctx);
    return (NULL);
}

void race_event_ctx_free(race_event_ctx_t *ctx)
{
    if (ctx->ssl_ctx) {
        race_ssl_ctx_free(ctx->ssl_ctx);
        SSL_free(ctx->ssl);
    }
    bufferevent_free(ctx->bev);
    free(ctx);
    ctx = NULL;
}

int race_module_init(race_module_t *m, race_t *r, struct event_base *base)
{
    return (m->init(&m->ctx, r, base));
}

void race_module_deinit(race_module_t *m)
{
    return (m->deinit(m->ctx));
}
