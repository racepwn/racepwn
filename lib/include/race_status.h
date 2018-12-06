#if !defined(_RACE_STATUS_H_)
#define _RACE_STATUS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RACE_OK = 0,
    RACE_ERROR,
    RACE_BAD_ARGS,
    RACE_MEM_ERROR,
    RACE_CONN_ERROR,
} race_status_t;

#ifdef __cplusplus
}
#endif

#endif // _RACE_STATUS_H_
