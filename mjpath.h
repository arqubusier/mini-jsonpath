#ifndef MJPATH_H
#define MJPATH_H

#include <stdlib.h>
#include "include/queue.h"

typedef struct{
    int size;
    char *ptr;
} strn_t;

typedef union{
    int as_int;
    strn_t as_strn;
}store_t;

enum store_tag{
    INT_TAG,
    STR_TAG
};

#define MJPATH_MAX_LEVEL 5

typedef struct{
    size_t size;
    const char** ptr;
} substrs_t;

LIST_HEAD(head_t, mjpath_target_t) target_list;

struct mjpath_target_t{
    int tag;
    int matches;
    size_t match_level;
    store_t store;
    const char* substrs[MJPATH_MAX_LEVEL];
    LIST_ENTRY(mjpath_target_t) neighbours; 
};
typedef struct mjpath_target_t mjpath_target_t;


enum states{
    MJPATH_START_S,
    MJPATH_KEY_S,
    MJPATH_KEY_ESCAPE_S,
    MJPATH_BEFORE_KEY_S,
    MJPATH_BEFORE_COLON_S,
    MJPATH_TYPE_S,
    MJPATH_TERMINAL_S,
    MJPATH_INT_S,
    MJPATH_FLOAT_S,
    MJPATH_STR_S,
    MJPATH_STR_ESCAPE_S
};

typedef struct{
    size_t keychar;
    size_t level;
    int sign;
    int state;
    int tmp_int;
    float tmp_float;
    struct head_t target_list;
    int stack[MJPATH_MAX_LEVEL];
} mjpath_context;

size_t mjpath_allocate(size_t n_targets, mjpath_target_t *targets, 
                    char** config, mjpath_context* ctx);
size_t mjpath_init(size_t n_targets, mjpath_target_t *targets, 
                    char** config, mjpath_context* ctx);
char* mjpath_get(char *json, size_t n_targets,
        mjpath_target_t *targets, mjpath_context* ctx);
void mjpath_debug(mjpath_context *ctx);
#endif //MJPATH_H