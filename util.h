#ifndef UTIL_H
#define UTIL_H

#define DEBUG_ON
#define INFO_ON

#ifdef DEBUG_ON
#include <stdio.h>
#define DEBUG(fmt, var) printf(__FILE__ "@l:%d in %s\t" #var":" fmt "\n", __LINE__, __func__, var);
#define DEBUGH(t) printf(__FILE__ "@l:%d in %s\t%s\n", __LINE__, __func__, t);
#else
#define DEBUG(x) ;
#endif

#ifdef INFO_ON
#include <stdio.h>
#define INFOV(fmt, var) printf(#var":" fmt "\n", var);
#else
#define INFOV(x) ;
#endif

#endif //UTIL_H
