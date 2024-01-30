#ifndef MINI_MUDUO_CURRENTTHREAD_H
#define MINI_MUDUO_CURRENTTHREAD_H

#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>


namespace CurrentThread {
    extern thread_local int t_cachedTid;
    extern thread_local const char *t_threadName;
    
    pid_t gettid() {
        return static_cast<pid_t> (::syscall(SYS_gettid));
    }

    void cacheTid() {
        if (t_cachedTid == 0) {
            t_cachedTid = gettid();
        }
    }
    
    int tid() {
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }
        return t_cachedTid;
    }


    bool isMainThread() {
    //TODO: why?
        return tid() == gettid();
    }

    const char* name() {
        return t_threadName;
    }
}

#endif
