#include "util.h"
#include "mjpath.h"
#include "include/queue.h"

int main(){
    char json1[] =
        "{ \"fff\" : 123}";
    char json2[] =
        "{ \"fff\" : 123, \"0123456789\": true}";
    
    const size_t n = 3;
    char s1[] = "int$.0123456789";
    char s2[] = "str  5$.abc\\..def.ghi";
    char s3[] = "int$.fff";
    char *conf[n];
    conf[0] = s1;
    conf[1] = s2;
    conf[2] = s3;
    mjpath_context mjpath_ctx;
    mjpath_target_t targets[n];
    mjpath_allocate(n, targets, conf);
    mjpath_init(n, targets, conf, &mjpath_ctx);
    mjpath_get(json2, &mjpath_ctx);

    printf("\n\nRESULTS\n");
    for(size_t i=0; i<n;++i){
        mjpath_debug_target(&targets[i]);
        printf("\n");
    }
    return 0;
}
