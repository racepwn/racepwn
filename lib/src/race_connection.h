#if !defined(_RACE_CONN_T)
#define _RACE_CONN_T

#include <errno.h>
#include <event2/http.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "race_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct evutil_addrinfo *info;
    bool use_ssl;
} race_conn_param_t;

typedef struct {
    evutil_socket_t sock;
    SSL_CTX *ssl_ctx;
    SSL *ssl;
} race_conn_t;

race_err_t *race_conn_param_init(race_conn_param_t **param, const char *host,
    int port, bool use_ssl);
void race_conn_param_free(race_conn_param_t *param);

race_err_t *race_conn_init(race_conn_t **conn, race_conn_param_t *param);
void race_conn_free(race_conn_t *conn);

#ifdef __cplusplus
}
#endif

#endif // _RACE_CONN_T
