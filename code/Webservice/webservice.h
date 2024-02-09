#ifndef MINI_WEB_SERVICE_WEBSERVICE_H
#define MINI_WEB_SERVICE_WEBSERVICE_H

#include "./../Log/log.h"
#include "./../SQLPool/sqlConnectionPool.h"
#include "epoller.h"

enum class TriggerMode { LEVEL, EDGE };

class WebService {
 public:
  WebService(int port, LogWriteMode logWriteMode, TriggerMode listenTriggerMode,
             TriggerMode connTriggerMode, int sqlNum, int threadNum);

  ~WebService();

 private:
  void InitEventMode(TriggerMode listenTriggerMode,
                     TriggerMode connTriggerMode);
  bool Listen();

  void Start();

 public:
  int Port;
  LogWriteMode logWriteMode;
  Epoller* epoller;

  int sqlNum;      // number of sql connections
  int threadNum;   // number of threads
  bool Lingering;  // whether to linger on close

  uint32_t listenEvent;
  uint32_t connEvent;
};

#endif  // MINI_WEB_SERVICE_WEBSERVICE_H