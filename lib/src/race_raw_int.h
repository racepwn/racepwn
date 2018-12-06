#if !defined(_RACE_RAW_INT_H_)
#define _RACE_RAW_INT_H_

#include <stdlib.h>

#include "../include/race_raw.h"
#include "race_error.h"
#include "race_int.h"

#ifdef __cplusplus
extern "C" {
#endif

struct race_raw_s {
    race_t *r;
    race_storage_t *storage;
};

#ifdef __cplusplus
}
#endif

#endif // _RACE_RAW_INT_H_
