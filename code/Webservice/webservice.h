#ifndef MINI_WEB_SERVICE_WEBSERVICE_H
#define MINI_WEB_SERVICE_WEBSERVICE_H

#include <unordered_map>

#include "./../Http/httpConn.h"
#include "./../Log/log.h"
#include "./../SQLPool/sqlConnectionPool.h"
#include "./../Timer/timer.h"
#include "epoller.h"

enum class TriggerMode { LEVEL, EDGE };

class WebService {
 public:
  WebService(int port, LogWriteMode logWriteMode, TriggerMode listenTriggerMode,
             TriggerMode connTriggerMode, int sqlNum, int threadNum, int TimeoutMS);

  ~WebService();

 private:
  void InitEventMode(TriggerMode listenTriggerMode,
                     TriggerMode connTriggerMode);
  bool Listen();

  void Start();

  void HandleListenEvent(int listenFd);
  void HandleCloseEvent(HttpConn* client);
  void HandleReadEvent(HttpConn* client);
  void HandleWriteEvent(HttpConn* client);

  void AddClient(int connFd, const sockaddr_in& clientAddr);

 public:
  int Port;
  int listenFd;
  LogWriteMode logWriteMode;
  Epoller* epoller;

  int sqlNum;      // number of sql connections
  int threadNum;   // number of threads
  bool Lingering;  // whether to linger on close

  uint32_t listenEvent;
  uint32_t connEvent;


  std::unordered_map<int, HttpConn> clientAddrMap;
  TimerHeap timerHeap;
  int TimeoutMS;
};

#endif  // MINI_WEB_SERVICE_WEBSERVICE_H