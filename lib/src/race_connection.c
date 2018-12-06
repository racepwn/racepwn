#include "race_connection.h"

static struct evutil_addrinfo *race_conn_get_addrinfo(const char *host,
    int port)
{
    struct evutil_addrinfo *info;
    char port_buf[6];
    struct evutil_addrinfo hints;

    evutil_snprintf(port_buf, sizeof(port_buf), "%d", (int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = EVUTIL_AI_ADDRCONFIG;
    if (evutil_getaddrinfo(host, port_buf, &hints, &info) != 0) {
        return NULL;
    }
    return info;
}

race_err_t *race_conn_param_init(race_conn_param_t **_param, const char *host,
    int port, bool use_ssl)
{
    race_conn_param_t *param;
    race_err_t *err = NULL;

    if (!(param = malloc(sizeof(*param)))) {
        err = race_err_base(RACE_ERR_MEMORY);
        goto fail;
    }
    param->use_ssl = use_ssl;
    param->info = race_conn_get_addrinfo(host, port);
    if (!param->info) {
        err = race_err_print(RACE_CONN_ERROR,
            "connection param init failure: "
            "cannot get address info for host %s, port %d",
            host, port);
        goto fail;
    }
    (*_param) = param;
    return NULL;

fail:
    if (param) {
        race_conn_param_free(param);
    }
    return err;
}

void race_conn_param_free(race_conn_param_t *param)
{
    if (!param) {
        return;
    }
    if (param->info) {
        evutil_freeaddrinfo(param->info);
    }
    free(param);
    param = NULL;
}

race_err_t *race_conn_init(race_conn_t **_conn, race_conn_param_t *param)
{
    race_conn_t *conn;
    race_err_t *err = NULL;
    struct evutil_addrinfo *info = param->info;

    if (!(conn = calloc(1, sizeof(*conn)))) {
        err = race_err_base(RACE_ERR_MEMORY);
        goto fail;
    }
    conn->sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (conn->sock <= 0) {
        err = race_err_print(RACE_CONN_ERROR,
            "connection init failure: "
            "cannot create socket: %s",
            strerror(errno));
        goto fail;
    }
    if (param->use_ssl) {
        const SSL_METHOD *method = SSLv23_client_method();
        if (!method) {
            err = race_err_print(RACE_CONN_ERROR,
                "connection init failure: cannot create ssl method");
            goto fail;
        }
        conn->ssl_ctx = SSL_CTX_new(method);
        if (!conn->ssl_ctx) {
            err = race_err_print(RACE_CONN_ERROR,
                "connection init failure: cannot create ssl ctx");
            goto fail;
        }
        SSL_CTX_set_verify(conn->ssl_ctx, SSL_VERIFY_NONE, NULL);
        conn->ssl = SSL_new(conn->ssl_ctx);
        if (!conn->ssl) {
            err = race_err_print(RACE_CONN_ERROR,
                "connection init failure: cannot create ssl");
            goto fail;
        }
    }
    (*_conn) = conn;
    return NULL;

fail:
    if (conn) {
        race_conn_free(conn);
    }
    return err;
}

void race_conn_free(race_conn_t *conn)
{
    if (!conn) {
        return;
    }
    if (conn->ssl_ctx) {
        if (conn->ssl) {
            SSL_free(conn->ssl);
        }
        SSL_CTX_free(conn->ssl_ctx);
    }
    if (conn->sock > 0) {
        close(conn->sock);
    }
    free(conn);
    conn = NULL;
}

__attribute__((constructor)) static void race_ssl_init() { SSL_library_init(); }
