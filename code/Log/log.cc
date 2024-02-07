#include "log.h"

const char* GetLevel(LogLevel level) {
  switch (level) {
    case LogLevel::INFO:
      return "INFO: ";
    case LogLevel::WARN:
      return "WARN: ";
    case LogLevel::ERROR:
      return "ERROR: ";
    case LogLevel::FATAL:
      return "FATAL: ";
    default:
      return "UNKNOWN: ";
  }
}

Log* Log::getInstance() {
  static Log log;
  return &log;
}

void Log::Init(LogWriteMode logWriteMode, LogLevel logLevel,
               std::string filePath, int maxQueueSize, int maxSplitLines,
               int maxLogBufferSize) {
  if (logWriteMode == LogWriteMode::Async) {
    logQueue =
        std::make_unique<BoundedBlockingQueue<std::string>>(maxQueueSize);
    writeThread = std::make_unique<std::thread>(&Log::AsyncWriteLog, this);
  }

  this->logLevel = logLevel;
  this->MaxSplitLines = maxSplitLines;
  this->MaxLogBufferSize = maxLogBufferSize;
  this->FilePath = filePath;
  this->Logbuf = new char[MaxLogBufferSize];
}

void Log::write(LogLevel level, const char* format, ...) {
  struct timeval now;
  gettimeofday(&now, nullptr);
  struct tm sysTime = *localtime(&now.tv_sec);

  // check if the day has changed or the log file has reached the maximum size
  // if so, create a new log file
  {
    std::lock_guard<std::mutex> lock(mutex);

    if (!fp || fileToday != sysTime.tm_mday ||
        (lineCount && (lineCount % MaxSplitLines == 0))) {
      char newFile[255];
      char tail[36] = {0};
      snprintf(tail, 36, "%04d_%02d_%02d", sysTime.tm_year + 1900,
               sysTime.tm_mon + 1, sysTime.tm_mday);

      if (fileToday != sysTime.tm_mday) {
        snprintf(newFile, 255, "%s/%s", FilePath.c_str(), tail);
        fileToday = sysTime.tm_mday;
        lineCount = 0;
      } else {
        snprintf(newFile, 255, "%s/%s-%d", FilePath.c_str(), tail,
                 (lineCount / MaxSplitLines));
      }

      if (fp != nullptr) {
        flush();
        fclose(fp);
      }
      fp = fopen(newFile, "a");
      if (fp == nullptr) {
        mkdir(FilePath.c_str(), 0777);
        fp = fopen(newFile, "a");
      }

      assert(fp != nullptr);
    }
  }

  // write log to file
  {
    std::lock_guard<std::mutex> lock(mutex);
    lineCount++;
    int n = snprintf(Logbuf, 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s",
                     sysTime.tm_year + 1900, sysTime.tm_mon + 1,
                     sysTime.tm_mday, sysTime.tm_hour, sysTime.tm_min,
                     sysTime.tm_sec, now.tv_usec, GetLevel(level));

    va_list valst;
    va_start(valst, format);

    int m = vsnprintf(Logbuf + n, MaxLogBufferSize - n - 1, format, valst);
    Logbuf[n + m] = '\n';
    Logbuf[n + m + 1] = '\0';

    std::string logStr = Logbuf;

    // write to file or logQueue
    if (logWriteMode == LogWriteMode::Async && logQueue && !logQueue->full()) {
      logQueue->put(logStr);
    } else {
      fputs(logStr.c_str(), fp);
    }

    va_end(valst);
  }
}

void Log::flush() {
  if (logWriteMode == LogWriteMode::Async) {
    // TODO: get all logs from logQueue and write to file
  }
  // std::lock_guard<std::mutex> lock(mutex);
  fflush(fp);
}

void Log::AsyncWriteLog() {
  while (true) {
    auto log = logQueue->pop();

    std::lock_guard<std::mutex> lock(mutex);
    fputs(log.c_str(), fp);
  }
}

Log::~Log() {
  if (logWriteMode == LogWriteMode::Async) {
    // TODO: stop writeThread
  }
  if (fp) {
    std::lock_guard<std::mutex> lock(mutex);
    fflush(fp);
    fclose(fp);
  }
}
