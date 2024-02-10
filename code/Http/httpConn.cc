#include "httpConn.h"

#include "./../Log/log.h"

std::atomic<int> HttpConn::userCount;

void HttpConn::Init(int fd, const sockaddr_in& addr) {
  sockfd = fd;
  address = addr;

  userCount++;
  LOG_INFO("HttpConn %d init success", sockfd);
}

void HttpConn::Close() {
  close(sockfd);
  userCount--;
  LOG_INFO("HttpConn %d close success", sockfd);
}

bool HttpConn::Read() {
  //TODO: read data from sockfd
  return true;
}

bool HttpConn::Write() {
  // TODO: write data to sockfd
  return true;
}

