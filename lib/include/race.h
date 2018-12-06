#if !defined(_RACE_H_)
#define _RACE_H_

#include <stdlib.h>

#include "race_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RACE_OPT_MODE_PARALELLS = 0,
    RACE_OPT_MODE_PIPELINE,
    RACE_OPT_URL,
    RACE_OPT_USE_SSL,
    RACE_OPT_LAST_CHUNK_DELAY_USEC,
    RACE_OPT_LAST_CHUNK_SIZE,
} race_options_t;

typedef struct race_s race_t;
typedef struct race_info_s race_info_t;

typedef struct {
    void *data;
    size_t len;
} race_response_t;

race_t *race_new(void);
void race_free(race_t *r);

race_status_t race_set_option(race_t *r, race_options_t opt, size_t option);
race_status_t race_run(race_t *r, race_info_t **info);

const char *race_strerror(race_t *r);

const char *race_info_get_status(race_info_t *info);
unsigned race_info_count(race_info_t *info);
race_response_t race_info_get_response(race_info_t *info, unsigned idx);

void race_info_free(race_info_t *info);

#ifdef __cplusplus
}
#endif

#endif // _RACE_H_
