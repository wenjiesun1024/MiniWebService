#include "./webservice.h"

WebService::WebService(int port, LogWriteMode logWriteMode,
                       TriggerMode triggerMode, int sqlNum, int threadNum)
    : Port(port),
      logWriteMode(logWriteMode),
      triggerMode(triggerMode),
      sqlNum(sqlNum),
      threadNum(threadNum) {
  // Init log
  Log::GetInstance()->Init(logWriteMode, LogLevel::INFO, "./log", 1000, 1000,
                           1000);

  // Init sql connection pool
  SqlConnectionPool::GetInstance()->InitPool("localhost", "root", "root",
   "yourdb", 3306, 8);

  LOG_INFO("WebService init success");
}