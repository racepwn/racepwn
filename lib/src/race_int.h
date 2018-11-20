#if !defined(_RACE_INT_H_)
#define _RACE_INT_H_

#include <time.h>
#include <event2/event.h>

#include "race_utils.h"
#include "race_module.h"

typedef enum {
    RACE_PARALELLS = 0,
    RACE_PIPELINE,
} race_mode_t;

typedef struct race_param_s {
    race_mode_t mode;
    struct timeval delay;
    size_t last_chunk_size;
} race_param_t;

typedef struct race_s {
    race_conn_param_t conn_param;
    race_param_t race_param;
    race_storage_t data;
} race_t;

#endif // _RACE_INT_H_
