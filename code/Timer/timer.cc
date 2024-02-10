#include "timer.h"

/*
     0
   1  2
  3 4 5 6
*/
int getFatherNode(int i) { return (i - 1) / 2; }

int getLeftNode(int i) { return 2 * i + 1; }

int getRightNode(int i) { return 2 * (i + 1); }

void TimerHeap::swapNode(int i, int j) {
  // std::cout << i << " " << j << " " << heap.size() << "\n";
  assert(i >= 0 && j >= 0 && i < heap.size() && j < heap.size());

  if (i == j) return;

  std::swap(heap[i], heap[j]);
  refMap[heap[i].id] = i;
  refMap[heap[j].id] = j;
}

void TimerHeap::addTimer(TimerNode timerNode) {
  auto it = refMap.find(timerNode.id);
  if (it == refMap.end()) {
    heap.push_back(timerNode);
    refMap[timerNode.id] = heap.size() - 1;
    adjustUp(heap.size() - 1);
  } else {
    int index = it->second;
    heap[index] = timerNode;
    adjustDown(index);
    adjustUp(index);
  }
}

void TimerHeap::delTimer(int id) {
  std::cout << id << " - delTimer\n";
  auto it = refMap.find(id);
  if (it == refMap.end()) {
    return;
  }

  int index = it->second;

  swapNode(index, heap.size() - 1);

  auto&& last = heap.back();
  refMap.erase(last.id);
  last.callbackFunc(&last);
  heap.pop_back();

  adjustDown(index);
  adjustUp(index);
}

void TimerHeap::adjustUp(int i) {
  while (i > 0 && heap[i] < heap[getFatherNode(i)]) {
    swapNode(i, getFatherNode(i));
    i = getFatherNode(i);
  }
}

void TimerHeap::adjustDown(int i) {
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

    swapNode(i, minIndex);

    i = minIndex;
  }
}

void TimerHeap::popTimer() {
  if (heap.empty()) {
    return;
  }

  delTimer(heap[0].id);
}

void TimerHeap::clear() {
  // TODO: clear
  refMap.clear();
  heap.clear();
}

void TimerHeap::tick() {
  if (heap.empty()) {
    return;
  }

  time_t now = time(nullptr);
  while (!heap.empty()) {
    if (heap[0].expireTime > now) {
      break;
    }

    heap[0].callbackFunc(&heap[0]);
    popTimer();
  }
}

void TimerHeap::adjust(int id, time_t newExpireTime) {
  auto it = refMap.find(id);
  if (it == refMap.end()) {
    return;
  }

  int index = it->second;
  heap[index].expireTime = newExpireTime;
  adjustDown(index);
  adjustUp(index);
}