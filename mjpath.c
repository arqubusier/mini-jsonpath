#include <stdint.h>
#include <string.h>

#include "mjpath.h"
#include "util.h"

int c2int(char c){
    switch (c){
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        default: return 0;
    }
}

/**
 * takes an array of 'config_expression', which  is a mini-jsonpath expression
 * prefixed with a string of one of two formats:
 *   - 'int<whitespaces>$', and
 *   - 'str<whitespace><whitespaces><decimal-number><whitespaces>$'.
 * With:
 *   - <whitespaces>: zero or more whitespaces,
 *   - <whitespace>: a whitespace.
 *   - <decimal-number>: a decimal number without delimiters
 *
 * The function sets up dynamic storage used in target structs according to the
 * config expressions
 * 
 * Returns the number of mjpath_targets that have been
 * successfully allocated.
 */
size_t mjpath_allocate(size_t n_targets, mjpath_target_t *targets, 
                    char** config){
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
            targets[i].tag=MJPATH_STR_TAG;
        }
        else{
            targets[i].tag=MJPATH_INT_TAG;
        }
    }
    return i;
}

/**
 * Initialize substring fields for each target in increasing order
 * starting from index 0.
 *
 * Returns the number of mjpath_targets initialized, breaking at the
 * first occureance of an error.
 *
 */
size_t mjpath_init(size_t n_targets, mjpath_target_t targets[],
                    char** config, mjpath_context* ctx){
    LIST_INIT(&ctx->target_list);
    ctx->state = MJPATH_START_S;
    size_t i = 0;
    for(;i<MJPATH_MAX_LEVEL;i++){
        ctx->stack[i] = MJPATH_STACK_OBJECT;
    }
    ctx->stack[0] = MJPATH_STACK_BOTTOM;
    ctx->level = 0;
    ctx->char_idx = 0;
    ctx->sign = 1;

    i = 0;
    for(;i<n_targets;++i){
        char * const pattern = strchr(config[i], '$');
        mjpath_target_t *target = &targets[i];
        if (pattern == NULL)
            return i;
        target->match_level = 0;
        target->matches = 0;

        size_t level=0;
        size_t c=1;
        target->substrs[0] = pattern;
        while (1){
            if(level+1>=MJPATH_MAX_LEVEL)
                    break;
            if      (pattern[c] == '.'){
                target->substrs[++level] = &pattern[++c];
            }
            else if (pattern[c] == '['){
                target->substrs[++level] = &pattern[c++];
            }
            else if (pattern[c] == '\0'){
                break;
            }
            else
                c++;
        }
        for (level++;level<MJPATH_MAX_LEVEL;level++){
            target->substrs[level] = NULL;
        }
        
        LIST_INSERT_HEAD(&ctx->target_list, target, neighbours);
    }
    return i;
}

void add_array_matches(mjpath_context* ctx){
  mjpath_target_t *target;
  mjpath_target_t *tmp;
  LIST_FOREACH_SAFE(target,
                    &ctx->target_list, neighbours, tmp)
    {
      if (target->match_level + 1 == ctx->level){
        target->match_level++;
        LIST_REMOVE(target, neighbours);
        LIST_INSERT_HEAD(&ctx->target_list, target,
                         neighbours);
      }
      else
        break;
    }
}

void handle_before_key_s(char c, mjpath_context* ctx){
    switch (c){
        case '\"':{
            mjpath_target_t *target;
            mjpath_target_t *tmp;
            LIST_FOREACH_SAFE(target,
                    &ctx->target_list, neighbours, tmp)
            {
                if (target->match_level + 1 == ctx->level){
                    target->match_level++;
                    LIST_REMOVE(target, neighbours);
                    LIST_INSERT_HEAD(&ctx->target_list, target,
                                neighbours);
                }
                else
                    break;
            }
            ctx->char_idx = 0;
            ctx->state = MJPATH_KEY_S;
            break;
        }
        case ' ': case '\t': case '\n':
        default: ;
    }
}

void handle_key_s(char c, mjpath_context *ctx){
    switch (c){
        case '\\':
            ctx->state = MJPATH_KEY_ESCAPE_S;
            break;
        case '\"':
            ctx->char_idx=0;
            ctx->state = MJPATH_TYPE_S;
            break;
        default:
        {
            mjpath_target_t *target;
            mjpath_target_t *tmp;
            LIST_FOREACH_SAFE(target,
                    &ctx->target_list, neighbours, tmp)
            {
                DEBUGH("a")
                DEBUG("%ld", ctx->char_idx)
                DEBUG("%ld", ctx->level)
                DEBUGH("b")
                if (target->match_level == ctx->level){
                    char ctarget = target->substrs
                        [ctx->level][ctx->char_idx];
                    if (ctarget == c){
                        LIST_REMOVE(target, neighbours);
                        LIST_INSERT_HEAD(&ctx->target_list, target,
                                        neighbours);
                    }
                    else{
                        target->match_level--;
                    }
                }
                else
                    break;
            }

            ctx->char_idx++;
        }
    }
}

int close_variable(char c, mjpath_context *ctx){
    int stack_e;
    switch (c){
        case ' ': case '\t': case '\n':
            ctx->state = MJPATH_NEXT_S;
            return 1;
        case ']': case '}':
            if (ctx->level == 1)
                ctx->state = MJPATH_DONE_S;                
            else{
                int stack_e =  ctx->stack[--ctx->level];
                if (stack_e != MJPATH_STACK_OBJECT)
                    ctx->stack[ctx->level] = stack_e+1;
                ctx->state = MJPATH_NEXT_S;
            }
            return 1;
        case ',':
            stack_e = ctx->stack[ctx->level];
            if (stack_e == MJPATH_STACK_OBJECT)
                ctx->state = MJPATH_BEFORE_KEY_S;
            else{
                ctx->stack[ctx->level]++;
                ctx->state = MJPATH_TYPE_S;
            }
            return 1;
        default:
            return 0;
    }
    
}

void handle_type_s(char c, mjpath_context *ctx){
    switch (c){
        // int
        case '-':
            ctx->sign = -1;
            ctx->tmp_int = 0;
            ctx->state = MJPATH_INT_S;
            break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            ctx->sign = 1;
            ctx->tmp_int = c2int(c);
            ctx->state = MJPATH_INT_S;
            break;
        // string
        case '\"':
            ctx->char_idx = 0;
            ctx->state = MJPATH_STR_S;
            break;
        // Other terminals
        case 't':
            ctx->tmp_int = 1;
            ctx->state = MJPATH_TERMINAL_S;
            break;
        case 'f':
            ctx->tmp_int = 0;
            ctx->state = MJPATH_TERMINAL_S;
            break;
        case 'n':
            ctx->tmp_int = -1;
            ctx->state = MJPATH_TERMINAL_S;
            break;
        //object
        case '{':
            ctx->stack[++ctx->level] = MJPATH_STACK_OBJECT;
            ctx->state = MJPATH_BEFORE_KEY_S;
            break;
        //array
        case '[':
            ctx->stack[++ctx->level] = 0;
            add_array_matches(ctx);
            ctx->state = MJPATH_TYPE_S;
            break;
        //empty array or object
        case '}': case ']':
            close_variable(c, ctx);
            break;
        //skip colon after key
        case ':': 
        case ' ': case '\t': case '\n':
        default: ;
    }

}

void update_matches(mjpath_context *ctx){
    mjpath_target_t *target;
    mjpath_target_t *tmp;
    LIST_FOREACH_SAFE(target, &ctx->target_list, neighbours, tmp){
        if (target->match_level == ctx->level){
            target->matches++;
            LIST_REMOVE(target, neighbours);
        }
        else return;
    }
}

void handle_int(char c, mjpath_context *ctx){
    if (close_variable(c, ctx)){
        mjpath_target_t *target;
        LIST_FOREACH(target, &ctx->target_list, neighbours){
            if (target->match_level == ctx->level)
                target->store.as_int = ctx->sign*ctx->tmp_int;
            else break;
        }
        update_matches(ctx);
    }
    else{
        mjpath_target_t *target;
        LIST_FOREACH(target, &ctx->target_list, neighbours){
            if (target->match_level == ctx->level)
                ctx->tmp_int = ctx->tmp_int*10 + c2int(c);
            return;
        }
    }
}

void handle_terminal(char c, mjpath_context *ctx){
    if (close_variable(c, ctx)){
        mjpath_target_t *target;
        LIST_FOREACH(target, &ctx->target_list, neighbours){
            if (target->match_level == ctx->level)
                target->store.as_int = ctx->tmp_int;
            else break;
        }
        update_matches(ctx);
    }
}

void handle_float(char c, mjpath_context *ctx){
    ;
}

void handle_string(char c, mjpath_context *ctx){
    if (c == '\"'){
        ctx->char_idx = 0;
        update_matches(ctx);
        ctx->state = MJPATH_NEXT_S;
    }
    else if (c == '\\'){
        ctx->state = MJPATH_STR_ESCAPE_S;
    }
    else{
        mjpath_target_t *target;
        LIST_FOREACH(target, &ctx->target_list, neighbours){
            if (target->match_level == ctx->level){
                size_t size = target->store.as_strn.size;
                if (ctx->char_idx < size)
                    target->store.as_strn.ptr[ctx->char_idx] = c;
                else
                    target->store.as_strn.ptr[size-1] = '\0';
            } else return;
        }
        ctx->char_idx++;
    }
}

void handle_string_escape(char c, mjpath_context *ctx){
    if (c == 'u'){
        ctx->state = MJPATH_STR_UNICODE0_S;
    }
    else{
        char escape_char;
        switch (c){
        case 'b':
            escape_char = '\b'; break;
        case 'f':
            escape_char = '\f'; break;
        case 'n':
            escape_char = '\n'; break;
        case 'r':
            escape_char = '\r'; break;
        case 't':
            escape_char = '\t'; break;
        case '\\': case '/': case '\"':
            escape_char = c; break;
        default:// Unknown escape sequence
            break;
        }

        mjpath_target_t *target;
        LIST_FOREACH(target, &ctx->target_list, neighbours){
            if (target->match_level == ctx->level){
                size_t size = target->store.as_strn.size;
                if (ctx->char_idx < size)
                    target->store.as_strn.ptr[ctx->char_idx] = escape_char;
                else
                    target->store.as_strn.ptr[size-1] = '\0';
            } else return;
        }
        ctx->char_idx++;
        ctx->state = MJPATH_STR_S;
    }
}

/* TODO */
void handle_unicode(char c, mjpath_context *ctx){
    if (ctx->state == MJPATH_STR_UNICODE0_S)
        ctx->state = MJPATH_STR_UNICODE1_S;
    else if (ctx->state == MJPATH_STR_UNICODE1_S)
        ctx->state = MJPATH_STR_UNICODE2_S;
    else if (ctx->state == MJPATH_STR_UNICODE2_S)
        ctx->state = MJPATH_STR_UNICODE3_S;
    else if (ctx->state == MJPATH_STR_UNICODE3_S)
        ctx->state = MJPATH_STR_S;

    mjpath_target_t *target;
    LIST_FOREACH(target, &ctx->target_list, neighbours){
        if (target->match_level == ctx->level){
            size_t size = target->store.as_strn.size;
            if (ctx->char_idx < size)
                target->store.as_strn.ptr[ctx->char_idx] = 'X';
            else
                target->store.as_strn.ptr[size-1] = '\0';
        } else return;
    }
    ctx->char_idx++;
}

void mjpath_parsec(const char c, mjpath_context* ctx){
    switch(ctx->state){
        case MJPATH_START_S:
            switch(c){
                case '{':
                    ctx->stack[++ctx->level] = MJPATH_STACK_OBJECT;
                    ctx->state = MJPATH_BEFORE_KEY_S;
                    break;
                case '[':
                    ctx->stack[++ctx->level] = 0;
                    add_array_matches(ctx);
                    ctx->state = MJPATH_TYPE_S;
                    break;
                case ' ': case '\t': case '\n':
                default:
                    ;
            }break;
        case MJPATH_BEFORE_KEY_S:
            handle_before_key_s(c, ctx); break;
        case MJPATH_KEY_S:
            handle_key_s(c, ctx); break;
        case MJPATH_TYPE_S:
            handle_type_s(c, ctx); break;
        case MJPATH_TERMINAL_S:
            handle_terminal(c, ctx); break;
        case MJPATH_INT_S:
            handle_int(c, ctx); break;
        case MJPATH_FLOAT_S:
            handle_float(c, ctx); break;
        case MJPATH_STR_S:
            handle_string(c, ctx); break;
        case MJPATH_NEXT_S:
            close_variable(c, ctx); break;
        case MJPATH_STR_ESCAPE_S:
            handle_string_escape(c, ctx);break;
        case MJPATH_STR_UNICODE0_S:
        case MJPATH_STR_UNICODE1_S:
        case MJPATH_STR_UNICODE2_S:
        case MJPATH_STR_UNICODE3_S:
            handle_unicode(c, ctx);

        default:
            ;
    }
}

char* mjpath_get(char *const json, mjpath_context* ctx){
    char *buf_p = json;
    for (; *buf_p!='\0'; ++buf_p){
        mjpath_parsec(*buf_p, ctx);
#ifdef DEBUG_ON
        printf("%s\n", json);
        size_t n = buf_p - json;
        for(size_t i=0; i<n; ++i)
            printf(" ");
        printf("^\n");
        mjpath_debug_ctx(ctx);
#endif
    }

    return buf_p;
}

void mjpath_debug_target(mjpath_target_t *target){
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
    printf("store: ");
    if (target->tag == MJPATH_INT_TAG)
        printf("%d\n", target->store.as_int);
    else{
        printf("%s\n", target->store.as_strn.ptr);
    }
}

/*
 *
 */
void mjpath_debug_ctx(mjpath_context *ctx){
    mjpath_target_t *target;
    
    printf("---Targets begin----:\n");
    LIST_FOREACH(target, &ctx->target_list, neighbours)
    {
        mjpath_debug_target(target);
        printf("\n");
    }
    printf("---Targets end----:\n");
    printf("\n");

    char state[22];
    state2str(ctx->state, state);
    INFOV("%s", state);
    INFOV("%ld", ctx->level);
    INFOV("%d", ctx->sign);
    INFOV("%d", ctx->tmp_int);
    INFOV("%f", ctx->tmp_float);
    INFOV("%ld", ctx->char_idx);
    printf("---Stack top----:\n");
    size_t i = 0;
    size_t top = ctx->level;
    for (;i<=top;i++){
        size_t j = top - i;
        int e = ctx->stack[j];
        if (e == MJPATH_STACK_OBJECT)
            printf("%ld obj\n", j);
        else if (e == MJPATH_STACK_BOTTOM)
            printf("STACK BOTTOM\n");
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
        case MJPATH_STR_UNICODE0_S:{
            char tmp[] = "MJPATH_STR_UNICODE0_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_STR_UNICODE1_S:{
            char tmp[] = "MJPATH_STR_UNICODE1_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_STR_UNICODE2_S:{
            char tmp[] = "MJPATH_STR_UNICODE2_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_STR_UNICODE3_S:{
            char tmp[] = "MJPATH_STR_UNICODE3_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_NEXT_S:{
            char tmp[] = "MJPATH_NEXT_S";
            strcpy(outbuf, tmp);
            break;}
        case MJPATH_DONE_S:{
            char tmp[] = "MJPATH_DONE_S";
            strcpy(outbuf, tmp);
            break;}
    }
}
