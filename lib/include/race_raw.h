#if !defined(_RACE_RAW_H_)
#define _RACE_RAW_H_

#include <stdbool.h>
#include <stdlib.h>

#include "race.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct race_raw_s race_raw_t;

race_raw_t *race_raw_new(race_t *r);
void race_raw_free(race_raw_t *rraw);
race_status_t race_raw_apply(race_raw_t *rraw);

race_status_t race_raw_set_url(race_raw_t *rraw, const char *url, bool use_ssl);

race_status_t race_raw_add_race_param(race_raw_t *rraw, unsigned count,
    const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // _RACE_RAW_H_
