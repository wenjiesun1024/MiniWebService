#include "threadPool.h"

#include <iostream>

#include "unistd.h"

// g++ *.cc ./../Log/log.cc -std=c++17 -O2 -Wall -g -lpthread -lmysqlclient
int main() {
  Log::GetInstance()->Init(LogWriteMode::Sync, LogLevel::INFO, "./log", 1000,
                           1000, 1000);

  ThreadPool pool(4);

  for (int i = 0; i < 100; i++) {
    pool.enqueue([i] {
      LOG_INFO("Task %d is running", i);
      sleep(1);
    });
  }

  sleep(10);
  return 0;
}
