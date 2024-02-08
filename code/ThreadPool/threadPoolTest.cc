#include <iostream>

#include "threadPool.h"
#include "unistd.h"

int main() {
  ThreadPool pool(4);

  for (int i = 0; i < 10; i++) {
    pool.enqueue([i] {
      std::cout << "Task " << i << " is running" << std::endl;
      sleep(1);
    });
  }

  sleep(20);
  return 0;
}
