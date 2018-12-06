#if !defined(_RACE_ERROR_H_)
#define _RACE_ERROR_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "../include/race_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RACE_ERR_UNKNOWN = 0,
    RACE_ERR_MEMORY,
} race_base_error_t;

typedef struct {
    char *str;
    race_status_t status;
    bool need_free;
} race_err_t;

race_err_t *race_err_print(race_status_t status, const char *format, ...);
race_err_t *race_err_base(race_base_error_t base_err);
void race_err_free(race_err_t *err);

#ifdef __cplusplus
}
#endif

#endif // _RACE_ERROR_H_
