#if !defined(_RACE_JOB_H_)
#define _RACE_JOB_H_

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>

#include "race_connection.h"
#include "race_int.h"
#include "race_storage.h"

#ifdef __cplusplus
extern "C" {
#endif

struct race_job_s;
struct race_conn_status_s;
typedef struct race_conn_status_s race_conn_status_t;

typedef enum {
    RACE_SEND_CONNECT = 0,
    RACE_SEND_DATA,
    RACE_WAIT_TIMEOUT,
    RACE_SEND_LAST_CHUNK,
} race_state_t;

typedef struct {
    struct race_job_s *job;
    race_buf_t *last_chunk;
    race_conn_status_t *conn_status;
    struct bufferevent *bev;
} race_job_item_t;

typedef struct race_job_s {
    race_state_t state;
    race_t *r;
    struct event_base *base;
    struct event *timer_ev;
    race_job_item_t *items;
    race_conn_t **conns;
    unsigned conn_count;
    unsigned conn_wait;
    unsigned conn_failure;
    unsigned conn_success;
} race_job_t;

race_job_t *race_job_new(race_t *r, race_info_t *info);
void race_job_free(race_job_t *j);

race_status_t race_job_run(race_job_t *j);

#ifdef __cplusplus
}
#endif

#endif // _RACE_JOB_H_
