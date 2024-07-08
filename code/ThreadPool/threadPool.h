#ifndef MINI_WEB_SERVICE_THREADPOOL_H
#define MINI_WEB_SERVICE_THREADPOOL_H

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <sstream>
#include <thread>
#include <vector>

#include "./../Log/log.h"

class ThreadPool {
 public:
  ThreadPool(int threadNum = 8) {
    assert(threadNum > 0);
    for (int i = 0; i < threadNum; i++) {
      threads.emplace_back([this] {
        while (!stop) {
          auto task = dequeue();
          if (!task.has_value()) {
            return;
          }
          task.value()();

          auto id = std::this_thread::get_id();
          std::stringstream ss;
          ss << "Thread " << id << " is running task\n";
          LOG_INFO(ss.str().c_str());
        }
      });
    }
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(mx);
      stop = true;
    }
    cv.notify_all();
    for (auto &thread : threads) {
      thread.join();
    }
  }

  std::optional<std::function<void()>> dequeue() {
    std::unique_lock<std::mutex> lock(mx);
    cv.wait(lock, [this] { return stop || !tasks.empty(); });
    if (stop && tasks.empty()) {
      return std::nullopt;
    }
    auto task = tasks.front();
    tasks.pop();
    return task;
  }

  template <class F, class... Args>
  void enqueue(F &&f, Args &&...args) {
    auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
    {
      std::unique_lock<std::mutex> lock(mx);
      tasks.emplace(task);
    }
    cv.notify_one();
  }

 private:
  std::vector<std::thread> threads;
  std::queue<std::function<void()>> tasks;
  std::condition_variable cv;
  std::mutex mx;
  bool stop = false;
};

#endif  // MINI_WEB_SERVICE_THREADPOOL_H