#include "race_raw_int.h"

race_raw_t *race_raw_new(race_t *r)
{
    race_raw_t *rraw;

    if (!(rraw = calloc(1, sizeof(*rraw)))) {
        return NULL;
    }
    if (!(rraw->storage = race_storage_new())) {
        goto fail;
    }
    rraw->r = r;
    return rraw;

fail:
    if (rraw) {
        race_raw_free(rraw);
    }
    return NULL;
}

void race_raw_free(race_raw_t *rraw)
{
    if (rraw->storage) {
        race_storage_free(rraw->storage);
    }
    free(rraw);
    rraw = NULL;
}

race_status_t race_raw_apply(race_raw_t *rraw)
{
    race_t *r = rraw->r;

    race_storage_move(r->storage, rraw->storage);
    return RACE_OK;
}

race_status_t race_raw_set_url(race_raw_t *rraw, const char *url,
    bool use_ssl)
{
    return race_set_url(rraw->r, url, use_ssl);
}

race_status_t race_raw_add_race_param(race_raw_t *rraw, unsigned count,
    const void *data, size_t len)
{
    race_t *r = rraw->r;

    if (count == 0) {
        r->err = race_err_print(RACE_BAD_ARGS,
            "variable \"count\" must be greater than zero");
        return r->err->status;
    }
    for (unsigned i = 0; i < count; i++) {
        race_buf_t *buf = race_buf_new(data, len, RACE_BUF_COPY_FLAG);
        if (!buf) {
            r->err = race_err_base(RACE_ERR_MEMORY);
            return RACE_MEM_ERROR;
        }
        race_storage_insert(rraw->storage, buf);
    }
    return RACE_OK;
}
