#if !defined(_RACE_H_)
#define _RACE_H_

#include <stdbool.h>
#include <stdlib.h>

typedef enum {
    RACE_OPTION_PARALELLS = 0,
    RACE_OPTION_PIPELINE,
    RACE_OPTION_DELAY_USEC,
    RACE_OPTION_LAST_CHUNK_SIZE,
} race_option_t;

typedef struct race_s race_t;
typedef struct rraw_s rraw_t;

race_t *race_job_new(void);
void race_job_free(race_t *r);

int race_set_option(race_t *r, race_option_t opt, size_t arg);

int race_run(race_t *r);

rraw_t *rraw_new(void);
int rraw_set_race(race_t *r, rraw_t *rraw);

int rraw_set_host(rraw_t *rraw, const char *uri, bool use_ssl);

int rraw_set_race_param(rraw_t *rraw,
    unsigned count, const void *data, size_t len);

#endif // _RACE_H_
