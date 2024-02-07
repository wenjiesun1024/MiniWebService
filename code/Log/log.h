#ifndef MINI_WEB_SERVICE_LOG_H
#define MINI_WEB_SERVICE_LOG_H

#include <sys/stat.h>
#include <sys/time.h>

#include <cstdarg>
#include <iostream>
#include <thread>

#include "./blockedQueue.h"

enum class LogWriteMode { Sync, Async };
enum LogLevel { INFO, WARN, ERROR, FATAL };

class Log {
 public:
  static Log* GetInstance();

  void Init(LogWriteMode logWriteMode, LogLevel logLevel, std::string filePath,
            int maxQueueSize, int maxSplitLines, int maxLogBufferSize);

  LogLevel GetLogLevel() const { return logLevel; }

  void write(LogLevel level, const char* format, ...);
  void flush();

 private:
  std::unique_ptr<std::thread> writeThread;
  std::unique_ptr<BoundedBlockingQueue<std::string>> logQueue;

  std::mutex mutex;
  LogLevel logLevel;
  LogWriteMode logWriteMode;

  std::string FilePath;
  int MaxSplitLines;
  int MaxLogBufferSize;

  FILE* fp;
  char* Logbuf;

  int lineCount;
  int fileToday;

 private:
  void AsyncWriteLog();

 private:
  Log() = default;
  ~Log();
  Log(const Log&) = delete;
  Log& operator=(const Log&) = delete;
};

#define LOG_BASE(level, format, ...)                           \
  {                                                            \
    if (level >= Log::GetInstance()->GetLogLevel()) {          \
      Log::GetInstance()->write(level, format, ##__VA_ARGS__); \
      if (level == LogLevel::FATAL) {                          \
        Log::GetInstance()->flush();                           \
        exit(1);                                               \
      }                                                        \
    }                                                          \
  }

#define LOG_INFO(format, ...) \
  { LOG_BASE(LogLevel::INFO, format, ##__VA_ARGS__); }
#define LOG_WARN(format, ...) \
  { LOG_BASE(LogLevel::WARN, format, ##__VA_ARGS__); }
#define LOG_ERROR(format, ...) \
  { LOG_BASE(LogLevel::ERROR, format, ##__VA_ARGS__); }
#define LOG_FATAL(format, ...) \
  { LOG_BASE(LogLevel::FATAL, format, ##__VA_ARGS__); }

#endif