#ifndef MINI_WEB_SERVICE_TIMER_H
#define MINI_WEB_SERVICE_TIMER_H

#include <assert.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <time.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

class TimerNode;

using TimeoutCallBack = std::function<void(TimerNode *)>;

class TimerNode {
 public:
  TimerNode(int id, time_t expireTime, TimeoutCallBack callbackFunc)
      : id(id), expireTime(expireTime), callbackFunc(callbackFunc) {}

  int id;  // sockfd
  time_t expireTime;
  TimeoutCallBack callbackFunc;
  sockaddr_in address;

  bool operator<(const TimerNode &t) { return expireTime < t.expireTime; }
};

class TimerHeap {
 public:
  TimerHeap() = default;
  ~TimerHeap() = default;

  void addTimer(TimerNode timerNode);
  void delTimer(int i);

  void clear();

  void tick();

  //   int GetNextTick();

 private:
  void popTimer();
  void adjustUp(int i);
  void adjustDown(int i);
  void swapNode(int i, int j);

 private:
  std::vector<TimerNode> heap;
  std::unordered_map<int, int> refMap;
};

#endif  // MINI_WEB_SERVICE_TIMER_H