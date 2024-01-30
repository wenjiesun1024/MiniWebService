#ifndef MINI_MUDUO_COUNTDOWNLATCH_H
#define MINI_MUDUO_COUNTDOWNLATCH_H

#include <mutex>
#include <condition_variable>

class CountDownLatch {
public:
    explicit CountDownLatch(int count);
    
    void wait();
    
    void countDown();
    
    int getCount() const;

 private:
    mutable std::mutex mutex;
    std::condition_variable cv;
    int count;
};

#endif // MINI_MUDUO_COUNTDOWNLATCH_H

