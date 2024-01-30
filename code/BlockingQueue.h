#ifndef MINI_MUDUO_BLOCKINGQUEUE_H
#define MINI_MUDUO_BLOCKINGQUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>

template<typename T>
class BlockingQueue {
public:
	void put(const T& x) {
        std::lock_guard<std::mutex> lock(mutex);
        deq.push_back(x);
		notEmpty.notify();
	}
	
    bool tryPop(T& x) {
        std::lock_guard<std::mutex> lock(mutex);
        if (deq.empty()) {
            return false;
        }
        x = deq.front();
        deq.pop_front();
        return true;
    }

    T waitAndPop() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this]{return !deq.empty();});
        auto t = deq.front();
        deq.pop_front();
        return t;
    }

	size_t size() const {
        std::lock_guard<std::mutex> lock(mutex);
        return deq.size();
	}

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex);
        return deq.empty();
    }

private:
	mutable std::mutex mutex;
	std::condition_variable notEmpty;
	std::deque<T> deq;
};

#endif
