#include "../include/race.h"
#include "race_int.h"

race_t *race_job_new(void)
{
    return (calloc(1, sizeof(race_t)));
}

void race_job_free(race_t *r)
{
    race_conn_param_deinit(&r->conn_param);
    free(r);
}

int race_set_option(race_t *r, race_option_t opt, size_t arg)
{
    unsigned p_uint = (unsigned)arg;

    switch (opt) {
    case RACE_OPTION_PARALELLS:
        r->race_param.mode = RACE_PARALELLS;
        break;
    case RACE_OPTION_PIPELINE:
        r->race_param.mode = RACE_PIPELINE;
        break;
    case RACE_OPTION_DELAY_USEC:
        r->race_param.delay = (struct timeval){
            .tv_sec = p_uint / 1000000,
            .tv_usec = p_uint % 1000000
        };
        break;
    case RACE_OPTION_LAST_CHUNK_SIZE:
        r->race_param.last_chunk_size = p_uint;
        break;
    default:
        return (-1);
    }
    return (0);
}

int race_run(race_t *r)
{
    int rc = 0;
    struct event_base *base;
    race_module_t module;

    if (!(base = event_base_new())) {
        return (-1);
    }
    switch (r->race_param.mode) {
    case RACE_PARALELLS:
        race_module_paraleels_init(&module);
        break;
    case RACE_PIPELINE:
        race_module_pipeline_init(&module);
        break;
    default:
        return (-1);
    }
    if (race_module_init(&module, r, base) < 0) {
        rc = -1;
        goto done;
    }
    rc = event_base_dispatch(base);
    race_module_deinit(&module);
done:
    while (!TAILQ_EMPTY(&r->data)) {
        race_storage_item_t *item = TAILQ_FIRST(&r->data);
        TAILQ_REMOVE(&r->data, item, node);
        race_storage_item_free(item);
    }
    event_base_free(base);
    return (rc);
}
