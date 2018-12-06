#include "race_error.h"

race_err_t base_errors[] = {
    [RACE_ERR_UNKNOWN] = {
        .str = "unknown error",
        .status = RACE_ERROR,
    },
    [RACE_ERR_MEMORY] = {
        .str = "not enough memory",
        .status = RACE_MEM_ERROR,
    },
};

race_err_t *race_err_print(race_status_t status, const char *format, ...)
{
    race_err_t *err;
    ssize_t size;
    va_list ptr;

    if (!(err = calloc(1, sizeof(*err)))) {
        goto fail;
    }
    va_start(ptr, format);
    size = vsnprintf(NULL, 0, format, ptr);
    if (size <= 0) {
        goto fail;
    }
    (*err) = (race_err_t){
        .status = status,
        .need_free = true,
        .str = calloc(1, size + 1),
    };
    if (!err->str) {
        goto fail;
    }
    va_start(ptr, format);
    vsnprintf(err->str, size + 1, format, ptr);
    va_end(ptr);
    return (err);

fail:
    if (err) {
        race_err_free(err);
    }
    return race_err_base(RACE_ERR_MEMORY);
}

race_err_t *race_err_base(race_base_error_t base_err)
{
    return &base_errors[base_err];
}

void race_err_free(race_err_t *err)
{
    if (err->need_free) {
        if (err->str) {
            free(err->str);
        }
        free(err);
    }
    err = NULL;
}
