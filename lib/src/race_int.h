#if !defined(_RACE_INT_H_)
#define _RACE_INT_H_

#include <string.h>

#include "../include/race.h"
#include "race_connection.h"
#include "race_error.h"
#include "race_job.h"
#include "race_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RACE_PARALELLS = 0,
    RACE_PIPELINE,
} race_mode_t;

typedef struct race_param_s {
    race_mode_t mode;
    struct timeval delay;
    size_t last_chunk_size;
} race_param_t;

typedef struct {
    bool use_ssl;
} race_cached_opt_t;

struct race_s {
    race_cached_opt_t cached_opt;
    race_param_t race_param;
    race_conn_param_t *conn_param;
    race_storage_t *storage;
    race_err_t *err;
};

typedef struct race_conn_status_s {
    race_buf_t *buf;
} race_conn_status_t;

struct race_info_s {
    char *global_status;
    race_conn_status_t *conn_statuses;
    unsigned count;
};

race_status_t race_set_url(race_t *r, const char *url, bool use_ssl);

race_info_t *race_info_new(unsigned count);

#ifdef __cplusplus
}
#endif

#endif // _RACE_INT_H_
