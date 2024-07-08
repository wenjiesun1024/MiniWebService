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
#include <mutex>
#include <unordered_map>
#include <vector>

#include "./../Log/log.h"

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
  void clear();
  void tick();
  void adjust(int id, time_t newExpireTime);

  bool isEmpty() const;

 private:
  void popTimer_withMutex();
  void adjustUp_withMutex(int i);
  void adjustDown_withMutex(int i);
  void swapNode_withMutex(int i, int j);
  void delTimer_withMutex(int i);

 private:
  mutable std::mutex mutex;
  std::vector<TimerNode> heap;
  std::unordered_map<int, int> refMap;
};

#endif  // MINI_WEB_SERVICE_TIMER_H