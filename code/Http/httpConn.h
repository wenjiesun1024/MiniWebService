#ifndef MINI_WEB_SERVICE_HTTPCONN_H
#define MINI_WEB_SERVICE_HTTPCONN_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>

enum class HttpCode {
  NO_REQUEST,
  GET_REQUEST,
  BAD_REQUEST,
  NO_RESOURCE,
  FORBIDDEN_REQUEST,
  FILE_REQUEST,
  INTERNAL_ERROR,
  CLOSED_CONNECTION
};

enum class HttpMethod {
  GET = 0,
  POST,
  HEAD,
  PUT,
  DELETE,
  TRACE,
  OPTIONS,
  CONNECT,
  PATCH
};

class HttpConn {
 public:
  HttpConn();
  ~HttpConn();

  void Init(int sockfd, const sockaddr_in& addr);
  // void closeConn(bool realClose = true);
  // void process();
  bool Read();
  bool Write();

  void Close();

  int GetFd() const { return epollFd; }

  int getPort() const { return ntohs(address.sin_port); }
  const char* getIP() const { return inet_ntoa(address.sin_addr); }
  sockaddr_in getAddr() const { return address; }

 private:
  struct sockaddr_in address;
  int sockfd;

  static std::atomic<int> userCount;
  int epollFd;
};

#endif