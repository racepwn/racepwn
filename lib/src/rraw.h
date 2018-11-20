#if !defined(_RRAW_H_)
#define _RRAW_H_

#include <event2/http.h>

#include "race_int.h"
#include "race_utils.h"

struct rraw_s {
    struct evhttp_uri *uri;
    race_storage_t data;
    bool use_ssl;
};

#endif // _RRAW_H_
