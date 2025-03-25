#include <pthread.h>
#include "common.h"
#include "ring_buffer.h"               

typedef struct bucket {
    key_type k;
    value_type v;
    struct bucket *next;
} bucket;