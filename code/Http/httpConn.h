#ifndef MINI_WEB_SERVICE_HTTPCONN_H
#define MINI_WEB_SERVICE_HTTPCONN_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iomanip>
#include <iostream>

enum class TriggerMode { LEVEL, EDGE };

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

enum class LineStatus { LINE_OK = 0, LINE_BAD, LINE_OPEN };
enum class CheckState {
  CHECK_STATE_REQUESTLINE = 0,
  CHECK_STATE_HEADER,
  CHECK_STATE_CONTENT
};

class HttpConn {
 public:
  HttpConn() = default;
  ~HttpConn() = default;

  void Init(int sockfd, const sockaddr_in& addr);
  // void closeConn(bool realClose = true);
  // void process();
  bool Read();
  bool Write();
  void Close();
  bool Process();

  int GetFd() const { return epollFd; }

  int getPort() const { return ntohs(address.sin_port); }
  const char* getIP() const { return inet_ntoa(address.sin_addr); }
  sockaddr_in getAddr() const { return address; }

  static void setConnectTriggerMode(TriggerMode mode) {
    connectTriggerMode = mode;
  }

 private:
  HttpCode ProcessRead();
  HttpCode parseRequestLine(char* text);
  HttpCode parseHeaders(char* text);
  HttpCode parseContent(char* text);
  LineStatus parseLine();
  char* getLine() { return readBuf + startLine; };
  HttpCode doRequest();

 private:
  struct sockaddr_in address;
  int sockfd;

  static std::atomic<int> userCount;
  static TriggerMode connectTriggerMode;
  int epollFd;

 private:
  static const int FILENAME_LEN = 200;
  static const int READ_BUFFER_SIZE = 2048;
  static const int WRITE_BUFFER_SIZE = 1024;

  char readBuf[READ_BUFFER_SIZE];
  long readIdx;
  char writeBuf[WRITE_BUFFER_SIZE];
  int writeIdx;

  int startLine;
  int checkedIdx;  // current character

  CheckState checkState;

  HttpMethod method;
  char *url, *version;

  const char* host;
  bool isLinger;
  int contentLength;

  std::string content;
};

#endif