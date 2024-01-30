#include "CountDownLatch.h"

CountDownLatch::CountDownLatch(int cnt) : count(cnt) {}

void CountDownLatch::wait() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [this]{return count == 0;});
}

void CountDownLatch::countDown() {
    std::lock_guard<std::mutex> lock(mutex);
    if (--count == 0) {
        cv.notify_all();
    }
}

int CountDownLatch::getCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return count;
}

