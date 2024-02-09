#ifndef MINI_WEB_SERVICE_HTTPCONN_H
#define MINI_WEB_SERVICE_HTTPCONN_H

#include <netinet/in.h>

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
  // bool read();
  // bool write();
  void Close();

  int GetFd() const { return epollFd; }

  // int getPort() const;
  // const char* getIP() const;
  // sockaddr_in getAddr() const;
  // static int userCount;

  int epollFd;
};

#endif