#include "timer.h"

/*
     0
   1  2
  3 4 5 6
*/
int getFatherNode(int i) { return (i - 1) / 2; }

int getLeftNode(int i) { return 2 * i + 1; }

int getRightNode(int i) { return 2 * (i + 1); }

void TimerHeap::swapNode_withMutex(int i, int j) {
  // std::cout << i << " " << j << " " << heap.size() << "\n";
  assert(i >= 0 && j >= 0 && i < heap.size() && j < heap.size());

  if (i == j) return;

  std::swap(heap[i], heap[j]);
  refMap[heap[i].id] = i;
  refMap[heap[j].id] = j;
}

void TimerHeap::addTimer(TimerNode timerNode) {
  LOG_INFO("addTimer id:%d expireTime:%ld", timerNode.id, timerNode.expireTime);

  std::unique_lock<std::mutex> lock(mutex);

  auto it = refMap.find(timerNode.id);
  if (it == refMap.end()) {
    heap.push_back(timerNode);
    refMap[timerNode.id] = heap.size() - 1;
    adjustUp_withMutex(heap.size() - 1);
  } else {
    int index = it->second;
    heap[index] = timerNode;
    adjustDown_withMutex(index);
    adjustUp_withMutex(index);
  }
}

void TimerHeap::delTimer_withMutex(int id) {
  LOG_INFO("delTimer id:%d at %ld", id, time(nullptr));

  auto it = refMap.find(id);
  if (it == refMap.end()) {
    return;
  }

  int index = it->second;

  swapNode_withMutex(index, heap.size() - 1);

  auto&& last = heap.back();
  refMap.erase(last.id);
  last.callbackFunc(&last);
  heap.pop_back();

  adjustDown_withMutex(index);
  adjustUp_withMutex(index);
}

void TimerHeap::adjustUp_withMutex(int i) {
  while (i > 0 && heap[i] < heap[getFatherNode(i)]) {
    swapNode_withMutex(i, getFatherNode(i));
    i = getFatherNode(i);
  }
}

void TimerHeap::adjustDown_withMutex(int i) {
  int n = heap.size();
  while (i < n) {
    int left = getLeftNode(i), right = getRightNode(i);
    int minIndex = i;
    if (left < n && heap[left] < heap[minIndex]) {
      minIndex = left;
    }
    if (right < n && heap[right] < heap[minIndex]) {
      minIndex = right;
    }

    if (minIndex == i) {
      break;
    }

    swapNode_withMutex(i, minIndex);

    i = minIndex;
  }
}

void TimerHeap::popTimer_withMutex() {
  if (heap.empty()) {
    return;
  }

  delTimer_withMutex(heap[0].id);
}

void TimerHeap::clear() {
  std::unique_lock<std::mutex> lock(mutex);

  refMap.clear();
  heap.clear();
}

void TimerHeap::tick() {
  // LOG_INFO("tick %ld", time(nullptr));
  std::unique_lock<std::mutex> lock(mutex);

  if (heap.empty()) {
    return;
  }

  time_t now = time(nullptr);
  while (!heap.empty()) {
    if (heap[0].expireTime > now) {
      break;
    }

    // heap[0].callbackFunc(&heap[0]);
    popTimer_withMutex();
  }
}

void TimerHeap::adjust(int id, time_t newExpireTime) {
  std::unique_lock<std::mutex> lock(mutex);

  auto it = refMap.find(id);
  if (it == refMap.end()) {
    return;
  }

  int index = it->second;
  heap[index].expireTime = newExpireTime;
  adjustDown_withMutex(index);
  adjustUp_withMutex(index);
}

bool TimerHeap::isEmpty() const {
  std::unique_lock<std::mutex> lock(mutex);
  return heap.empty();
}