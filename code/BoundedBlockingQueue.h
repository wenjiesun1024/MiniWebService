#ifndef MINI_MUDUO_BOUNDEDBLOCKINGQUEUE_H 
#define MINI_MUDUO_BOUNDEDBLOCKINGQUEUE_H

#include <condition_variable>
#include <mutex>
#include <assert.h>

template<typename T>
class BoundedBlockingQueue {
public:
	explicit BoundedBlockingQueue(size_t maxSize) : maxSize(maxSize) {}

	void put(const T& x) {		
		std::unique_lock<std::mutex> lock(mutex);
		notFull.wait(lock, [this]{return deq.size() < maxSize;});

		deq.push_back(x);
		
		notEmpty.notify();
	}


	T pop() {
		std::unique_lock<std::mutex> lock(mutex);
		notEmpty.wait(lock, [this]{return !deq.empty();});
		
		T t(deq.front());
		deq.pop_front();
		notFull.notify();
		return t;
	}

	bool tryPop(T& x) {
		std::unique_lock<std::mutex> lock(mutex);
		if (deq.empty()) {
			return false;
		}
		x = deq.front();
		deq.pop_front();
		notFull.notify();
		return true;
	}

	bool empty() const {
		std::unique_lock<std::mutex> lock(mutex);
		return deq.empty();
	}

	bool full() const {
		std::unique_lock<std::mutex> lock(mutex);
		return deq.size() == maxSize;
	}


	size_t size() const {
		std::unique_lock<std::mutex> lock(mutex);
		return deq.size();
	}

	size_t capacity() const {
		std::unique_lock<std::mutex> lock(mutex);
		return maxSize;
	}

private:
	mutable std::mutex mutex;
	std::condition_variable notEmpty, notFull;
	std::deque<T> deq;
	size_t maxSize;
};

#endif
