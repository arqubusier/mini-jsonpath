#ifndef UTIL_H
#define UTIL_H

#define DEBUG_ON

#ifdef DEBUG_ON
#include <stdio.h>
#define DEBUG(fmt, var) printf(__FILE__ "@l:%d in %s\t" #var":" fmt "\n", __LINE__, __func__, var);

#else
#define DEBUG(x) ;
#endif

#endif //UTIL_H
