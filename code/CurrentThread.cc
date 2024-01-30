#include "CurrentThread.h"


namespace CurrentThread {
    thread_local int t_cachedTid = 0;
    thread_local const char *t_threadName = "unknow";

    // __thread char t_tidString[32];
    // __thread int t_tidStringLength = 6;
}