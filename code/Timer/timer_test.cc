#include "timer.h"

#include <iostream>

// g++ *.cc ./../Log/log.cc -std=c++17 -O2 -Wall -g -lpthread -lmysqlclient
int main() {
  Log::GetInstance()->Init(LogWriteMode::Sync, LogLevel::INFO, "./log", 1000,
                           1000, 1000);
  TimerHeap timerHeap;

  time_t now = time(nullptr);
  std::vector<int> v;
  for (int i = 0; i < 100; i++) v.push_back(i);

  std::random_shuffle(v.begin(), v.end());

  for (auto i : v) {
    timerHeap.addTimer(TimerNode(i, (time_t)(now + i), [](TimerNode* p) {
      std::cout << "Timer " << p->id << " expired" << std::endl;
    }));
  }

  while (!timerHeap.isEmpty()) {
    timerHeap.tick();
  }
}