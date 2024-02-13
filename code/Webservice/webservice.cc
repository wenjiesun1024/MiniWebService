#include "./webservice.h"

#include <netinet/in.h>
#include <netinet/ip.h>
#include <strings.h>
#include <sys/socket.h>

#include "./../Log/log.h"

WebService::WebService(int port, LogWriteMode logWriteMode,
                       TriggerMode listenTriggerMode,
                       TriggerMode connTriggerMode, int sqlNum, int threadNum,
                       int TimeoutMS)
    : Port(port),
      logWriteMode(logWriteMode),
      sqlNum(sqlNum),
      epoller(new Epoller()),
      threadNum(threadNum),
      TimeoutMS(TimeoutMS) {
  // Init log
  Log::GetInstance()->Init(logWriteMode, LogLevel::INFO, "./log", 1000, 1000,
                           1000);
  LOG_INFO("Log init success");

  // Init sql connection pool
  SqlConnectionPool::GetInstance()->InitPool("localhost", "root", "root",
                                             "yourdb", 3306, 8);
  // InitmysqlResult();
  LOG_INFO("SqlConnectionPool init success");

  // Init event mode
  InitEventMode(listenTriggerMode, connTriggerMode);
  LOG_INFO("Event mode init success");

  // Init ThreadPool
  threadPool = new ThreadPool(threadNum);
  LOG_INFO("ThreadPool init success");

  // Init listenFd
  if (!Listen()) {
    LOG_ERROR("Listen failure");
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

  HttpConn::setConnectTriggerMode(connTriggerMode);
}

bool WebService::Listen() {
  // Create socket
  listenFd = socket(PF_INET, SOCK_STREAM, 0);
  if (listenFd < 0) {
    LOG_ERROR("Create socket failure");
    return false;
  }

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
  if (bind(listenFd, (struct sockaddr *)&address, sizeof(address)) < 0) {
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

void WebService::Start() {
  bool timeout = false;
  bool stop = false;
  while (!stop) {
    int eventNum = epoller->wait(-1);  // FIXME: timeout
    LOG_INFO("eventNum: %d", eventNum);
    for (int i = 0; i < eventNum; ++i) {
      int fd = epoller->GetEventFd(i);
      uint32_t events = epoller->GetEvents(i);
      if (fd == listenFd) {
        HandleListenEvent(fd);
      } else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {  // close
        HandleCloseEvent(clientAddrMap[fd]);
      } else if (events & EPOLLIN) {  // read
        HandleReadEvent(clientAddrMap[fd]);
      } else if (events & EPOLLOUT) {  // write
        HandleWriteEvent(clientAddrMap[fd]);
      }
    }
  }
  LOG_INFO("WebService stop");
}

void WebService::HandleListenEvent(int fd) {
  do {
    LOG_INFO("HandleListenEvent %d", fd);
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int connFd =
        accept(listenFd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    if (connFd < 0) {
      LOG_ERROR("Accept failure");
      return;
    }
    // if (HttpConn::userCount >= MAX_FD) {
    //   SendError_(fd, "Server busy!");
    //   LOG_WARN("Clients is full!");
    //   return;
    // }
    AddClient(connFd, clientAddr);
  } while (listenEvent & EPOLLET);  // ET mode
}

void WebService::HandleCloseEvent(HttpConn *client) {
  assert(client);
  epoller->DelFd(client->GetFd());
  client->Close();
  LOG_INFO("Client[%d] quit!", client->GetFd());
}

void WebService::HandleReadEvent(HttpConn *client) {
  ExtendTimer(client);
  threadPool->enqueue(std::bind(&WebService::OnRead, this, client));
}

void WebService::HandleWriteEvent(HttpConn *client) {
  ExtendTimer(client);
  threadPool->enqueue(std::bind(&WebService::OnWrite, this, client));
}

void WebService::AddClient(int connFd, const sockaddr_in &clientAddr) {
  clientAddrMap[connFd] = new HttpConn();
  clientAddrMap[connFd]->Init(connFd, clientAddr);
  if (TimeoutMS > 0) {
    time_t cur = time(nullptr);
    timerHeap.addTimer(TimerNode{
        connFd, cur + TimeoutMS,
        std::bind(&WebService::HandleCloseEvent, this, clientAddrMap[connFd])});
  }
  epoller->AddFd(connFd, connEvent);
  SetNonBlocking(connFd);

  LOG_INFO("Client[%d] in!", connFd);
}

WebService::~WebService() {
  LOG_INFO("WebService destruct");
  // TODO: Implement this function
}

void WebService::ExtendTimer(HttpConn *client) {
  if (TimeoutMS > 0) {
    time_t cur = time(nullptr);

    timerHeap.adjust(client->GetFd(), cur + TimeoutMS);
  }
}

void WebService::OnRead(HttpConn *client) {
  LOG_INFO("OnRead");
  if (client->Read() <= 0) {
    // TODO: handle read failure
  }
  OnProcess(client);
}

void WebService::OnWrite(HttpConn *client) {
  LOG_INFO("OnWrite");
  int writeErrno = 0;
  bool ret = client->Write(writeErrno);

  if (ret) {
    epoller->ModFd(client->GetFd(), connEvent | EPOLLIN);
    return;
  }

  if (writeErrno == EAGAIN) {
    epoller->ModFd(client->GetFd(), connEvent | EPOLLOUT);
    return;
  }

  if (client->keepAlice()) {
    OnProcess(client);
    return;
  }
  HandleCloseEvent(client);
}

void WebService::OnProcess(HttpConn *client) {
  // TODO: implement this function
  if (client->Process()) {
    epoller->ModFd(client->GetFd(), connEvent | EPOLLOUT);  // TODO: why?
  } else {
    epoller->ModFd(client->GetFd(), connEvent | EPOLLIN);
  }
}
