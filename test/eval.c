#include <stdio.h>
#include <stdlib.h>
#include "../util.h"
#include "../mjpath.h"
#include "../include/queue.h"


int main(int argc, char* argv[]){
    if (argc != 3){
        printf("Usage: %s INPUT_FILE CONFIG_EXPRESSION\n", argv[0]);
        return 1;
    }

    char* input = argv[1];
    char* path = argv[2];

    FILE* fp = fopen(input, "r");
    if (!fp){
        perror("error opening input file");
        return 2;
    }

    mjpath_context ctx;
    mjpath_target_t target;
    mjpath_allocate(1, &target, &path);
    mjpath_init(1, &target, &path, &ctx);

    int c;
    size_t i = 0;
    while( (c = fgetc(fp)) != EOF ){
        mjpath_parsec(c, &ctx);
        printf("%c %ld\n",c, i);
        mjpath_debug_ctx(&ctx);
        i++;
    }

    printf("\n");
    printf("\n");
    printf("DONE\n");
    mjpath_debug_target(&target);
    printf("\n");
}
