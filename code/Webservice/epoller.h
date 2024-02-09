#ifndef MINI_WEB_SERVICE_EPOLLER_H
#define MINI_WEB_SERVICE_EPOLLER_H

#include <sys/epoll.h>

#include <vector>

class Epoller {
 public:
  Epoller(int maxEvent = 1024);
  ~Epoller();

  bool AddFd(int fd, uint32_t events);
  bool ModFd(int fd, uint32_t events);
  bool DelFd(int fd);

  int wait(int timeout);

  int GetEventFd(size_t i) const;
  uint32_t GetEvents(size_t i) const;

 private:
  int epollFd;
  std::vector<struct epoll_event> events;
};

void SetNonBlocking(int fd);

#endif