#include "util.h"
#include "mjpath.h"
#include "include/queue.h"

int main(){
    char z[] = "Hello, world!";
    DEBUG("%s",z);
    
    const int n = 3;
    char s1[] = "int$xyz";
    char s2[] = "str  5$abc.def.ghi";
    char s3[] = "int$fff";
    char *conf[n];
    conf[0] = s1;
    conf[1] = s2;
    conf[2] = s3;
    mjpath_context mjpath_ctx;
    mjpath_target_t targets[n];
    mjpath_allocate(n, targets, conf, &mjpath_ctx);
    mjpath_init(n, targets, conf, &mjpath_ctx);

    mjpath_debug(&mjpath_ctx);
    return 0;
}
