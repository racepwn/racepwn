#if !defined(_RACE_STORAGE_H_)
#define _RACE_STORAGE_H_

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#define RACE_BUF_COPY_FLAG (1 << 0)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct race_buf_s {
    STAILQ_ENTRY(race_buf_s)
    node;
    void *data;
    size_t len;
    unsigned flags;
} race_buf_t;

typedef struct race_storage_s {
    STAILQ_HEAD(, race_buf_s);
    unsigned count;
} race_storage_t;

race_buf_t *race_buf_new(const void *data, size_t len, unsigned flags);
void race_buf_free(race_buf_t *buf);
race_buf_t *race_buf_realloc_left(race_buf_t *buf, size_t left);
race_buf_t *race_buf_merge(race_buf_t *buf1, race_buf_t *buf2);
race_buf_t *race_buf_add(race_buf_t *buf, const void *data, size_t len);

race_storage_t *race_storage_new(void);
void race_storage_free(race_storage_t *storage);
void race_storage_insert(race_storage_t *storage, race_buf_t *buf);
void race_storage_move(race_storage_t *dst, race_storage_t *src);

#ifdef __cplusplus
}
#endif

#endif // _RACE_STORAGE_H_
