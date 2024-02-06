// #include <iostream>

// #include "timer.h"

// int main() {
//   TimerHeap timerHeap;

//   std::vector<int> v;
//   for (int i = 0; i < 100; i++) v.push_back(i);

//   std::random_shuffle(v.begin(), v.end());

//   for (auto i : v) {
//     timerHeap.addTimer(TimerNode(i, (time_t)i, [](TimerNode* p) {
//       std::cout << "Timer " << p->id << " expired" << std::endl;
//     }));
//   }

//   for (auto i : timerHeap.heap) {
//     std::cout << i.id << " ";
//     i.callbackFunc(&i);
//   }

//   for (auto [i, j] : timerHeap.refMap) {
//     std::cout << i << " " << timerHeap.heap[j].id << std::endl;
//   }
//   for (int i = 0; i < 10; i++) {
//     timerHeap.delTimer(v[i]);
//   }

//   for (int i = 10; i < 100; i++) {
//     timerHeap.popTimer();
//     std::cout << timerHeap.heap.size() << std::endl;
//   }
// }