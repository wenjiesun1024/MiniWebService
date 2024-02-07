#include "log.h"

int main() {
  Log::GetInstance()->Init(LogWriteMode::Async, LogLevel::ERROR, "./log", 1000,
                           1000, 1000);

  while (1) {
    LOG_INFO("hello world");
    LOG_WARN("hello world1");
    LOG_ERROR("hello world2");
  }
  return 0;
}