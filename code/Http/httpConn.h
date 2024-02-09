#ifndef MINI_WEB_SERVICE_HTTPCONN_H
#define MINI_WEB_SERVICE_HTTPCONN_H

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

  void init(int sockfd, const sockaddr_in& addr);
  void closeConn(bool realClose = true);
  void process();
  bool read();
  bool write();

  int getFd() const;
  int getPort() const;
  const char* getIP() const;
  sockaddr_in getAddr() const;

  static int m_userCount;
  static int m_epollFd;
};

#endif