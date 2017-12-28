
#include <string.h>

#include "mjpath.h"
#include "util.h"
/*
 *
 *  "int$....."
 *  "str 10$.."
 *
 *  Returns the number of mjpath_targets that have been
 *  successfully allocated.
 */
size_t mjpath_allocate(size_t n_targets, mjpath_target_t *targets, 
                    char** config, mjpath_context *ctx){
    size_t i = 0;
    for(;i<n_targets;++i){
        const char* target_conf = config[i];
        if (strncmp(target_conf, "str", 3) == 0){
            size_t store_n = atoi(target_conf + 3);
            if (store_n == 0)
                return i;
            
            targets[i].store.as_strn.size = store_n;
            targets[i].store.as_strn.ptr = malloc(store_n);
            if (targets[i].store.as_strn.ptr == NULL)
                return i;
            targets[i].tag=STR_TAG;
        }
        else{
            targets[i].tag=INT_TAG;
        }
        DEBUG("%d", targets[i].tag)
    }
    return i;
}

/*
 * Initialize substring fields for each target in increasing order
 * starting from index 0.
 *
 * Returns the number of mjpath_targets initialized, breaking at the
 * first occureance of an error.
 */
size_t mjpath_init(size_t n_targets, mjpath_target_t targets[],
                    char** config, mjpath_context* ctx){
    LIST_INIT(&ctx->target_list);
    ctx->state = MJPATH_START_S;
    size_t i = 0;
    for(;i<n_targets;++i){
        char * const pattern = strchr(config[i], '$') + 1;
        mjpath_target_t *target = &targets[i];
        if (pattern == NULL)
            return i;
        target->match_level = 0;
        target->matches = 0;

        size_t level=0;
        size_t c=0;
        while (1){
            if(level+1>=MJPATH_MAX_LEVEL)
                    break;
            if      (pattern[c] == '.'){
                target->substrs[level++] = &pattern[++c];
                char const* x = &pattern[c-1];
                DEBUG("%s", x);
                DEBUG("%s",target->substrs[level-1]) 
            }
            else if (pattern[c] == '['){
                target->substrs[level++] = &pattern[c++];
            }
            else if (pattern[c] == '\0'){
                break;
            }
            else
                c++;
        }
        for (;level<MJPATH_MAX_LEVEL;level++){
            target->substrs[level] = NULL;
        }
        
        DEBUG("%d", target->tag)
        LIST_INSERT_HEAD(&ctx->target_list, target, neighbours);
    }
    return i;
}

int open_array(mjpath_context* ctx){
    ;
}

int open_object(mjpath_context* ctx){
    ;
}
int open_next_elem(mjpath_context* ctx){
    ;
}
int close_variable(mjpath_context* ctx, char c){
    switch (c){
        case ']':
            //TODO IF ARRAY ELSE 
            return 0;
        case '}':
            //TODO IF ARRAY ELSE 
            return 0;
        case ',':
            return open_next_elem(ctx);
        case ' ': case '\t': case '\n':
        default:
            return ctx->state;
    }
}

char* mjpath_get(char *const json, size_t n_targets,
        mjpath_target_t *targets, mjpath_context* ctx){
    char *buf_p = json;
    for (; *buf_p!='\0'; ++buf_p){
        char c = *buf_p;
        switch(ctx->state){
            case MJPATH_START_S:
                switch(c){
                    case '{':
                        ctx->state = open_object(ctx); break;
                    case '[':
                        ctx->state = open_array(ctx); break;
                    case ' ': case '\t': case '\n':
                    default:
                        ;
                }break;
            case MJPATH_BEFORE_KEY_S:
                switch (c){
                    case '\"':
                        ctx->state = MJPATH_KEY_S;
                    case ' ': case '\t': case '\n':
                    default:
                        ;
                }break;
            case MJPATH_KEY_S:
                switch (c){
                    case '\\':
                        ctx->state = MJPATH_KEY_ESCAPE_S; break;
                    case '\"':
                        ctx->state = MJPATH_TYPE_S; break;
                    default:
                        ;
                }break;
            case MJPATH_TYPE_S:
                switch (c){
                    // int
                    case '-':
                        ctx->sign = -1;
                        ctx->tmp_int = 0;
                        ctx->state = MJPATH_INT_S; break;
                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                        ctx->sign = 1;
                        ctx->tmp_int = c;
                        ctx->state = MJPATH_INT_S; break;
                    // string
                    case '\"':
                        ctx->state = MJPATH_STR_S; break;
                    // Other terminals
                    case 't':
                        ctx->tmp_int = 1;
                        ctx->state = MJPATH_TERMINAL_S; break;
                    case 'f':
                        ctx->tmp_int = 0;
                        ctx->state = MJPATH_TERMINAL_S; break;
                    case 'n':
                        ctx->tmp_int = -1;
                        ctx->state = MJPATH_TERMINAL_S; break;
                    //object
                    case '{':
                        ctx->state = open_object(ctx); break;
                    //array
                    case '[':
                        ctx->state = open_array(ctx); break;
                    case ':': 
                    case ' ': case '\t': case '\n':
                    default:
                        ;
                }break;
            case MJPATH_TERMINAL_S:
                ctx->state = close_variable(ctx, c); break;
            case MJPATH_INT_S:
                ctx->state = close_variable(ctx, c); break;
            case MJPATH_FLOAT_S:
                ctx->state = close_variable(ctx, c); break;
            case MJPATH_STR_S:
                if (c == '\\'){
                    ctx->state = MJPATH_KEY_ESCAPE_S;
                    break;
                }
                ctx->state = close_variable(ctx, c); break;
            default:
                ;
        }
    }

    return buf_p;
}

/*
 *
 */
void mjpath_debug(mjpath_context *ctx){
    mjpath_target_t *target;
    
    printf("---Targets begin----:\n");
    LIST_FOREACH(target, &ctx->target_list, neighbours)
    {
        INFOV("%d",target->tag);
        INFOV("%d",target->matches);
        INFOV("%ld",target->match_level);
        int level = 0;
        printf("substrs\n");
        for (;level<MJPATH_MAX_LEVEL; level++){
            if (target->substrs[level] == NULL)
                    break;
            char const*c = target->substrs[level];
            int escape =0;
            for (;(escape || *c!='.') && *c!='\0';c++){
                if (*c=='\\')
                    escape = 1;
                else
                    escape = 0;
                printf("%c", *c);
            }
            printf(" . ");
        }
        printf("\n");
        printf("\n");
    }
    printf("---Targets end----:\n");

    char state[22];
    state2str(ctx->state, state);
    INFOV("%s", state);
    printf("---Stack top----:\n");
    size_t i = 0;
    for (;i<MJPATH_MAX_LEVEL;i++){
        size_t j = (MJPATH_MAX_LEVEL-1) - i;
        int e = ctx->stack[j];
        if (e == MJPATH_STACK_OBJECT)
            printf("%ld obj\n", j);
        else
            printf("%ld arr:%d\n", j, e);
    }
    printf("---Stack bottom----:\n");
}

void state2str(int state, char outbuf[static 22]){
    switch(state){
        case MJPATH_START_S:{
            char tmp[] = "MJPATH_START_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_KEY_S:{
            char tmp[] = "MJPATH_KEY_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_KEY_ESCAPE_S:{
            char tmp[] = "MJPATH_KEY_ESCAPE_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_BEFORE_KEY_S:{
            char tmp[] = "MJPATH_BEFORE_KEY_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_BEFORE_COLON_S:{
            char tmp[] = "MJPATH_BEFORE_COLON_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_TYPE_S:{
            char tmp[] = "MJPATH_TYPE_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_TERMINAL_S:{
            char tmp[] = "MJPATH_TERMINAL_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_INT_S:{
            char tmp[] = "MJPATH_INT_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_FLOAT_S:{
            char tmp[] = "MJPATH_FLOAT_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_STR_S:{
            char tmp[] = "MJPATH_STR_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_STR_ESCAPE_S:{
            char tmp[] = "MJPATH_STR_ESCAPE_S";
            strcpy(outbuf, tmp);
            break;}
    }
}
