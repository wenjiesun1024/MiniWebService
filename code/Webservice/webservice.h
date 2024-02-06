#ifndef MINI_WEB_SERVICE_WEBSERVICE_H
#define MINI_WEB_SERVICE_WEBSERVICE_H

enum class TriggerMode { LEVEL, EDGE };

enum class LogWriteMode { Sync, Async };

class WebService {
 public:
  WebService(int port, LogWriteMode logWriteMode, TriggerMode triggerMode,
             int sqlNum, int threadNum);

 public:
  int Port;
  LogWriteMode logWriteMode;
  TriggerMode triggerMode;

  int sqlNum;     // number of sql connections
  int threadNum;  // number of threads
};

#endif  // MINI_WEB_SERVICE_WEBSERVICE_H