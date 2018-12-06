#include "race_storage.h"

race_buf_t *race_buf_new(const void *data, size_t len, unsigned flags)
{
    race_buf_t *buf;

    if (!(buf = malloc(sizeof(*buf)))) {
        goto fail;
    }
    buf->flags = flags;
    buf->len = len;
    if (flags & RACE_BUF_COPY_FLAG) {
        if (!(buf->data = malloc(len))) {
            goto fail;
        } else {
            memcpy(buf->data, data, len);
        }
    } else {
        buf->data = (void *)data;
    }
    return buf;

fail:
    if (buf) {
        free(buf);
    }
    return NULL;
}

void race_buf_free(race_buf_t *buf)
{
    if (!buf) {
        return;
    }
    if (buf->data && (buf->flags & RACE_BUF_COPY_FLAG)) {
        free(buf->data);
    }
    free(buf);
    buf = NULL;
}

race_buf_t *race_buf_realloc_left(race_buf_t *buf, size_t left)
{
    void *data;

    if (!(data = malloc(left))) {
        return NULL;
    }
    memcpy(data, buf->data + (buf->len - left), left);
    if (buf->flags & RACE_BUF_COPY_FLAG) {
        free(buf->data);
    }
    buf->data = data;
    buf->len = left;
    return buf;
}

race_storage_t *race_storage_new(void)
{
    race_storage_t *storage;

    if (!(storage = malloc(sizeof(*storage)))) {
        return NULL;
    }
    STAILQ_INIT(storage);
    storage->count = 0;
    return storage;
}

void race_storage_free(race_storage_t *storage)
{
    if (!storage) {
        return;
    }
    while (!STAILQ_EMPTY(storage)) {
        race_buf_t *buf = STAILQ_FIRST(storage);
        STAILQ_REMOVE_HEAD(storage, node);
        race_buf_free(buf);
    }
    free(storage);
    storage = NULL;
}

void race_storage_insert(race_storage_t *storage, race_buf_t *buf)
{
    STAILQ_INSERT_TAIL(storage, buf, node);
    storage->count++;
}

void race_storage_move(race_storage_t *dst, race_storage_t *src)
{
    while (!STAILQ_EMPTY(src)) {
        race_buf_t *buf = STAILQ_FIRST(src);
        STAILQ_REMOVE_HEAD(src, node);
        STAILQ_INSERT_TAIL(dst, buf, node);
    }
    dst->count += src->count;
    STAILQ_INIT(src);
    src->count = 0;
}

race_buf_t *race_buf_merge(race_buf_t *buf1, race_buf_t *buf2)
{
    void *data_merged;

    if (buf1 == NULL) {
        return buf2;
    } else if (buf2 == NULL) {
        return buf1;
    }

    data_merged = realloc(buf1->data, buf1->len + buf2->len);
    if (!data_merged) {
        return NULL;
    }
    buf1->data = data_merged;
    memcpy(buf1->data + buf1->len, buf2->data, buf2->len);
    buf1->len += buf2->len;
    race_buf_free(buf2);
    return buf1;
}

race_buf_t *race_buf_add(race_buf_t *buf, const void *data, size_t len)
{
    void *new_data;

    if (!buf) {
        return race_buf_new(data, len, RACE_BUF_COPY_FLAG);
    }
    if (!(new_data = realloc(buf->data, buf->len + len))) {
        return NULL;
    }
    buf->data = new_data;
    memcpy(buf->data + buf->len, data, len);
    return buf;
}
