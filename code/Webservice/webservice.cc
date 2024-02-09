#include "./webservice.h"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <strings.h>
#include <sys/socket.h>

#include "./../Log/log.h"

WebService::WebService(int port, LogWriteMode logWriteMode,
                       TriggerMode listenTriggerMode,
                       TriggerMode connTriggerMode, int sqlNum, int threadNum)
    : Port(port),
      logWriteMode(logWriteMode),
      sqlNum(sqlNum),
      epoller(new Epoller()),
      threadNum(threadNum) {
  // Init log
  Log::GetInstance()->Init(logWriteMode, LogLevel::INFO, "./log", 1000, 1000,
                           1000);
  LOG_INFO("Log init success");

  // Init sql connection pool
  // SqlConnectionPool::GetInstance()->InitPool("localhost", "root", "root",
  //                                            "yourdb", 3306, 8);
  // LOG_INFO("SqlConnectionPool init success");

  InitEventMode(listenTriggerMode, connTriggerMode);

  // Init listenFd
  if (Listen()) {
    LOG_ERROR("WebService init failure");
  }

  LOG_INFO("WebService init success");
}

void WebService::InitEventMode(TriggerMode listenTriggerMode,
                               TriggerMode connTriggerMode) {
  listenEvent = EPOLLRDHUP | EPOLLIN;
  connEvent = EPOLLONESHOT | EPOLLRDHUP | EPOLLIN;
  if (listenTriggerMode == TriggerMode::LEVEL) {
    listenEvent |= EPOLLET;
  }

  if (connTriggerMode == TriggerMode::LEVEL) {
    connEvent |= EPOLLET;
  }
}

bool WebService::Listen() {
  // Create socket
  int listenFd = socket(PF_INET, SOCK_STREAM, 0);
  assert(listenFd >= 0);

  // Set socket address
  struct sockaddr_in address;
  bzero(&address, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(Port);

  // Set socket options
  int optval = 1;
  if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) <
      0) {
    LOG_ERROR("Set socket options failure");
    return false;
  }

  struct linger optLinger = {0, 0};
  if (Lingering) optLinger = {1, 1};

  if (setsockopt(listenFd, SOL_SOCKET, SO_LINGER, &optLinger,
                 sizeof(optLinger)) < 0) {
    LOG_ERROR("Set socket options failure");
    return false;
  }

  // Bind socket
  if (bind(listenFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
    LOG_ERROR("Bind socket failure");
    return false;
  }

  // Listen
  if (listen(listenFd, 5) < 0) {
    LOG_ERROR("Listen failure");
    return false;
  }

  // Add listenFd to epoll
  if (!epoller->AddFd(listenFd, listenEvent)) {
    LOG_ERROR("Add listenFd to epoll failure");
    // close(listenFd);
    return false;
  }

  SetNonBlocking(listenFd);

  LOG_INFO("Add listenFd to epoll success");
  return true;
}

void Start() {
  bool timeout = false;
  bool stop = false;
  while (!stop) {
    int eventNum = epoller->wait(timeout ? -1 : 0);
    for (int i = 0; i < eventNum; ++i) {
      int fd = epoller->GetEventFd(i);
      uint32_t events = epoller->GetEvents(i);
      if (fd == listenFd) {
        if (events & EPOLLIN) {
          if (!Listen()) {
            LOG_ERROR("Listen failure");
          }
        }
      }
    }
  }
}