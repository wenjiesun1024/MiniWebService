#include "./webservice.h"

#include <iostream>

#include "./../Pool/sqlConnectionPool.h"

WebService::WebService(int port, LogWriteMode logWriteMode,
                       TriggerMode triggerMode, int sqlNum, int threadNum)
    : Port(port),
      logWriteMode(logWriteMode),
      triggerMode(triggerMode),
      sqlNum(sqlNum),
      threadNum(threadNum) {
  // Init sql connection pool
  SqlConnectionPool::GetInstance()->InitPool("localhost", "root", "root",
                                             "yourdb", 3306, 8);
}