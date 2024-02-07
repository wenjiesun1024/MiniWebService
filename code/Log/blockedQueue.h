#ifndef MINI_WEB_SERVICE_BLOCKED_QUEUE_H
#define MINI_WEB_SERVICE_BLOCKED_QUEUE_H

#include <assert.h>

#include <condition_variable>
#include <deque>
#include <mutex>

template <typename T>
class BoundedBlockingQueue {
 public:
  explicit BoundedBlockingQueue(size_t maxSize) : maxSize(maxSize) {}

  void put(const T& x) {
    std::unique_lock<std::mutex> lock(mutex);
    notFull.wait(lock, [this] { return deq.size() < maxSize; });

    deq.push_back(x);

    notEmpty.notify_one();
  }

  T pop() {
    std::unique_lock<std::mutex> lock(mutex);
    notEmpty.wait(lock, [this] { return !deq.empty(); });

    T t(deq.front());
    deq.pop_front();
    notFull.notify_one();
    return t;
  }

  bool tryPop(T& x) {
    std::unique_lock<std::mutex> lock(mutex);
    if (deq.empty()) {
      return false;
    }
    x = deq.front();
    deq.pop_front();
    notFull.notify_one();
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

#endif  // MINI_WEB_SERVICE_BLOCKED_QUEUE_H