#include "race_utils.h"

race_storage_item_t *race_storage_item_new(void *data, size_t len)
{
    race_storage_item_t *item;

    if (!(item = malloc(sizeof(*item)))) {
        return (NULL);
    }
    (*item) = (race_storage_item_t){
        .data = data,
        .len = len,
    };
    return (item);
}

void race_storage_item_free(race_storage_item_t *item)
{
    if (!item) {
        return;
    }
    if (item->data) {
        free(item->data);
    }
    free(item);
    item = NULL;
}

void race_storage_realloc_left(race_storage_item_t *item, size_t left)
{
    void *new_data;

    if (!(new_data = malloc(left))) {
        return;
    }
    memcpy(new_data, item->data + (item->len - left), left);
    free(item->data);
    item->data = new_data;
    item->len = left;
}

int race_conn_param_init(race_conn_param_t *param, struct evhttp_uri *uri)
{
    const char *scheme, *host;
    int port, scheme_port = 0;
    bool use_ssl = false;

    if (!(host = evhttp_uri_get_host(uri))) {
        return (-1);
    }
    if ((scheme = evhttp_uri_get_scheme(uri))) {
        if (!strcasecmp("http", scheme)) {
            scheme_port = 80;
        } else if (!strcasecmp("https", scheme)) {
            scheme_port = 443;
            use_ssl = true;
        }
    }
    if ((port = evhttp_uri_get_port(uri)) < 0) {
        if (!scheme_port) {
            return (-1);
        }
        port = scheme_port;
    }
    (*param) = (race_conn_param_t){
        .host = strdup(host),
        .port = port,
        .use_ssl = use_ssl,
    };
    return (0);
}

void race_conn_param_deinit(race_conn_param_t *param)
{
    if (param->host) {
        free(param->host);
    }
}

SSL_CTX *race_ssl_ctx_new(void)
{
    SSL_CTX *ssl_ctx = SSL_CTX_new(SSLv23_client_method());
    if (!ssl_ctx) {
        return (NULL);
    }
    SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_NONE, NULL);
    return (ssl_ctx);
}

evutil_socket_t
race_tcp_sock_create(const char *hostname, ev_uint16_t port, 
    struct evutil_addrinfo **answer)
{
    char port_buf[6];
    struct evutil_addrinfo hints;
    int err;
    evutil_socket_t sock;

    evutil_snprintf(port_buf, sizeof(port_buf), "%d", (int)port);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = EVUTIL_AI_ADDRCONFIG;

    err = evutil_getaddrinfo(hostname, port_buf, &hints, answer);
    if (err != 0) {
        return -1;
    }

    sock = socket((*answer)->ai_family,
        (*answer)->ai_socktype,
        (*answer)->ai_protocol);
    if (sock < 0)
        return -1;
    return sock;
}

void race_ssl_ctx_free(SSL_CTX *ctx)
{
    SSL_CTX_free(ctx);
}

__attribute__((constructor)) static void race_ssl_init()
{
    SSL_library_init();
}
