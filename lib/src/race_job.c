#include "race_job.h"

#define RACE_READ_CHUNK 16384

static void race_timeout_cb(evutil_socket_t fd, short what, void *_ctx);
static void race_read_cb(struct bufferevent *bev, void *ctx);
static void race_write_cb(struct bufferevent *bev, void *ctx);
static void race_event_cb(struct bufferevent *bev, short what, void *ctx);

static void race_timer_init(race_job_item_t *item)
{
    race_job_t *j = item->job;
    j->timer_ev = evtimer_new(j->base, race_timeout_cb, j);
    if (!j->timer_ev) {
        j->r->err = race_err_print(RACE_ERROR, "race timer: init failure");
        goto fail;
    }
    if (evtimer_add(j->timer_ev, &j->r->race_param.delay) == -1) {
        j->r->err = race_err_print(RACE_ERROR,
            "race timer: cannot add event to event base");
        goto fail;
    }
    return;

fail:
    event_base_loopexit(j->base, NULL);
    return;
}

static void race_timeout_cb(evutil_socket_t fd, short what, void *ctx)
{
    race_job_t *j = ctx;

    for (unsigned i = 0; i < j->conn_count; i++) {
        race_job_item_t *item = &j->items[i];
        bufferevent_write(item->bev,
            item->last_chunk->data, item->last_chunk->len);
    }
    j->conn_wait = j->conn_success;
}

static void race_read_cb(struct bufferevent *bev, void *ctx)
{
    race_job_item_t *item = ctx;
    race_t *r = item->job->r;
    void *data;
    size_t size;

    if (!(data = malloc(RACE_READ_CHUNK))) {
        r->err = race_err_base(RACE_ERR_MEMORY);
        event_base_loopbreak(item->job->base);
        return;
    }
    while ((size = bufferevent_read(bev, data, RACE_READ_CHUNK))) {
        race_buf_t *new_buf = race_buf_add(item->conn_status->buf, data, size);
        if (!new_buf) {
            r->err = race_err_base(RACE_ERR_MEMORY);
            event_base_loopbreak(item->job->base);
            return;
        }
        item->conn_status->buf = new_buf;
    }
    if (data) {
        free(data);
    }
}

static void race_write_cb(struct bufferevent *bev, void *ctx)
{
    race_job_item_t *item = ctx;
    race_job_t *j = item->job;

    j->conn_wait--;
    if (j->conn_wait) {
        return;
    }
    switch (j->state) {
    case RACE_SEND_DATA:
        if (j->r->race_param.last_chunk_size != 0) {
            race_timer_init(item);
            j->state = RACE_SEND_LAST_CHUNK;
        } else {
            bufferevent_disable(item->bev, EV_WRITE);
        }
        break;
    case RACE_SEND_LAST_CHUNK:
        bufferevent_disable(item->bev, EV_WRITE);
        break;
    default:
        event_base_loopbreak(j->base);
        break;
    }
}

static void race_write_event(race_job_item_t *item, short what_write)
{
    race_job_t *j = item->job;

    switch (what_write) {
    default:
        if (j->state == RACE_SEND_CONNECT) {
            j->conn_failure++;
            j->conn_wait--;
        }
        break;
    }
}

static void race_event_cb(struct bufferevent *bev, short what, void *ctx)
{
    race_job_item_t *item = ctx;
    race_job_t *j = item->job;

    if (what & BEV_EVENT_WRITING) {
        race_write_event(item, what ^ BEV_EVENT_WRITING);
    } else if (what == BEV_EVENT_CONNECTED) {
        j->conn_success++;
    } else {
        return;
    }

    if (j->conn_success + j->conn_failure == j->conn_count) {
        switch (j->state) {
        case RACE_SEND_CONNECT:
            j->state = RACE_SEND_DATA;
            break;
        case RACE_SEND_LAST_CHUNK:
        case RACE_WAIT_TIMEOUT:
            event_base_loopexit(j->base, NULL);
            break;
        case RACE_SEND_DATA:
            if (j->r->race_param.last_chunk_size != 0) {
                race_timer_init(item);
            }
            break;
        }
    }
}

race_job_t *race_job_new(race_t *r, race_info_t *info)
{
    race_buf_t *buf;
    race_job_t *j;
    unsigned conn_count = r->storage->count;

    j = calloc(1, sizeof(*j));
    if (!j) {
        r->err = race_err_base(RACE_ERR_MEMORY);
        goto fail;
    }
    j->state = RACE_SEND_CONNECT;
    j->r = r;
    j->conn_count = conn_count;
    j->items = calloc(conn_count, sizeof(*j->items));
    j->conns = calloc(conn_count, sizeof(*j->conns));
    if (!j->items || !j->conns) {
        r->err = race_err_base(RACE_ERR_MEMORY);
        goto fail;
    }
    j->base = event_base_new();
    if (!j->base) {
        r->err = race_err_print(RACE_ERROR,
            "race job: cannot initialize base event loop");
        goto fail;
    }
    for (unsigned i = 0; i < conn_count; i++) {
        race_conn_t *conn = NULL;
        r->err = race_conn_init(&conn, r->conn_param);
        j->conns[i] = conn;
    }
    buf = STAILQ_FIRST(r->storage);
    for (unsigned i = 0; i < conn_count; i++) {
        race_conn_t *conn = j->conns[i];
        race_job_item_t *item = &j->items[i];
        item->job = j;
        if (r->conn_param->use_ssl) {
            item->bev = bufferevent_openssl_socket_new(j->base,
                conn->sock, conn->ssl, BUFFEREVENT_SSL_CONNECTING, 0);
        } else {
            item->bev = bufferevent_socket_new(j->base, conn->sock, 0);
        }
        if (!item->bev) {
            r->err = race_err_print(RACE_ERROR,
                "race job: cannot create bufferevent socket");
            goto fail;
        }
        bufferevent_setcb(item->bev, race_read_cb, race_write_cb, race_event_cb,
            item);
        bufferevent_enable(item->bev, EV_READ | EV_WRITE);
        size_t left = r->race_param.last_chunk_size;
        if (left) {
            if (left > buf->len) {
                left = buf->len - 1;
            }
            bufferevent_write(item->bev, buf->data, buf->len - left);
            item->last_chunk = race_buf_realloc_left(buf, left);
        } else {
            bufferevent_write(item->bev, buf->data, buf->len);
        }
        bufferevent_socket_connect(item->bev, r->conn_param->info->ai_addr,
            r->conn_param->info->ai_addrlen);
        evutil_make_socket_nonblocking(conn->sock);
        item->conn_status = &info->conn_statuses[i];
        buf = STAILQ_NEXT(buf, node);
    }
    j->conn_wait = conn_count;
    return j;

fail:
    if (j) {
        race_job_free(j);
    }
    return NULL;
}

void race_job_free(race_job_t *j)
{
    if (!j) {
        return;
    }
    if (j->timer_ev) {
        event_free(j->timer_ev);
    }
    if (j->items) {
        for (unsigned i = 0; i < j->conn_count; i++) {
            race_job_item_t *item = &j->items[i];
            if (!item) {
                continue;
            }
            bufferevent_free(item->bev);
        }
        free(j->items);
    }
    if (j->conns) {
        for (unsigned i = 0; i < j->conn_count; i++) {
            race_conn_t *conn = j->conns[i];
            if (!conn) {
                continue;
            }
            race_conn_free(conn);
        }
        free(j->conns);
    }
    if (j->base) {
        event_base_free(j->base);
    }
    free(j);
    j = NULL;
}

race_status_t race_job_run(race_job_t *j)
{
    race_t *r = j->r;
    if (event_base_dispatch(j->base) == -1) {
        r->err = race_err_print(RACE_ERROR,
            "race job: cannot initialize base event loop");
    }
    if (r->err) {
        return r->err->status;
    }
    return RACE_OK;
}
