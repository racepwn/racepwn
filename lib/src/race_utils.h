#if !defined(_RACE_UTILS_H)
#define _RACE_UTILS_H

#include <event2/http.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

struct race_storage_item_s {
    TAILQ_ENTRY(race_storage_item_s)
    node;
    void *data;
    size_t len;
};

TAILQ_HEAD(race_storage_s, race_storage_item_s);

typedef struct race_storage_item_s race_storage_item_t;
typedef struct race_storage_s race_storage_t;

typedef struct race_conn_param_s {
    char *host;
    int port;
    bool use_ssl;
} race_conn_param_t;

race_storage_item_t *race_storage_item_new(void *data, size_t len);
void race_storage_item_free(race_storage_item_t *item);
void race_storage_realloc_left(race_storage_item_t *item, size_t left);

int race_conn_param_init(race_conn_param_t *param, struct evhttp_uri *uri);
void race_conn_param_deinit(race_conn_param_t *param);

evutil_socket_t
race_tcp_sock_create(const char *hostname,
     ev_uint16_t port, struct evutil_addrinfo **answer);

SSL_CTX *race_ssl_ctx_new(void);
void race_ssl_ctx_free(SSL_CTX *ctx);

#endif // _RACE_UTILS_H
