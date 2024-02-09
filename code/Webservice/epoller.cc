#include "epoller.h"

#include <assert.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>

Epoller::Epoller(int maxEvent) : epollFd(epoll_create(512)), events(maxEvent) {
  assert(epollFd >= 0);
}

Epoller::~Epoller() { close(epollFd); }

bool Epoller::AddFd(int fd, uint32_t events) {
  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool Epoller::ModFd(int fd, uint32_t events) {
  struct epoll_event ev;
  ev.data.fd = fd;
  ev.events = events;
  return epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool Epoller::DelFd(int fd) {
  return epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, nullptr) == 0;
}

int Epoller::wait(int timeout) {
  return epoll_wait(epollFd, &events[0], static_cast<int>(events.size()),
                    timeout);
}

int Epoller::GetEventFd(size_t i) const { return events[i].data.fd; }

uint32_t Epoller::GetEvents(size_t i) const { return events[i].events; }

void SetNonBlocking(int fd) {
  int flag = fcntl(fd, F_GETFL);
  assert(flag >= 0);
  int ret = fcntl(fd, F_SETFL, flag | O_NONBLOCK);
  assert(ret >= 0);
}