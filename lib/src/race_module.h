#if !defined(_RACE_MODULE_H_)
#define _RACE_MODULE_H_

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/dns.h>
#include <openssl/ssl.h>
#include <unistd.h>

#include "race_int.h"
#include "race_utils.h"

typedef struct race_ctx_s race_ctx_t;
typedef struct race_s race_t;

typedef enum {
    RACE_SEND_CONNECT = 0,
    RACE_SEND_DATA,
    RACE_WAIT_TIMEOUT,
    RACE_SEND_LAST_CHUNK,
} race_state_t;

typedef struct {
    race_ctx_t *ctx;
    race_storage_item_t *write_storage;
} race_arg_t;

typedef struct {
    race_arg_t arg;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
    struct bufferevent *bev;
} race_event_ctx_t;

typedef struct race_ctx_s {
    race_state_t state;
    race_t *r;
    struct event_base *base;
    race_event_ctx_t **event_ctx;
    struct event *timer_ev;
    unsigned event_ctx_count;
    unsigned buffers_left;
    unsigned buffers_connected;
    unsigned buffers_err;
} race_ctx_t;

typedef struct {
    void *ctx;
    int (*init)(void **, race_t *, struct event_base *);
    void (*deinit)(void *);
} race_module_t;

int race_module_init(race_module_t *m, race_t *r, struct event_base *base);
void race_module_deinit(race_module_t *m);

void race_module_paraleels_init(race_module_t *m);
void race_module_pipeline_init(race_module_t *m);

race_event_ctx_t *
race_event_ctx_init(race_ctx_t *race_ctx, race_storage_item_t *write_storage);

void race_event_ctx_free(race_event_ctx_t *ctx);

#endif // _RACE_MODULE_H_
