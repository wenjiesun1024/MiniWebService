#ifndef MINI_MUDUO_MUTEX_H
#define MINI_MUDUO_MUTEX_H

#include <assert.h>
#include "CurrentThread.h"
#include <mutex>


class MutexLock {
public:
    MutexLock() : holder(0) {}

    ~MutexLock() {
        assert(holder == 0);
    }

    void lock() {
        mx.lock();
        assignHolder();
    }

    void unlock() {
        unassignHolder();
        mx.unlock();
    }

    bool isLockedByThisThread() const {
        return holder == CurrentThread::tid();
    }
    
    // void assertLocked() const {
    //     assert(isLockedByThisThread());
    // }
    void unassignHolder() {holder = 0;}
    void assignHolder() {holder = CurrentThread::tid();}

    std::mutex* getMutex() {
        return &mx;
    }

    pid_t getholder() {
        return holder;
    }

private:
    pid_t holder;
    std::mutex mx;
    // mutex is nocopyable, because it has no copy constructor and assignment operator
};


#endif // MINI_MUDUO_MUTEX_H
