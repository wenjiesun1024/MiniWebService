#ifndef MINI_WEB_SERVICE_HTTPCONN_H
#define MINI_WEB_SERVICE_HTTPCONN_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <string>

#include "./../SQLPool/sqlConnectionPoolRAII.h"

void InitmysqlResult();

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

  void Init(int sockfd, int epollfd, const sockaddr_in& addr);
  void Reset();
  // void InitmysqlResult();

  void CloseConn(bool realClose = true);
  bool Read();
  bool Write(int& writeErrno);
  void Close();
  bool Process();

  int GetFd() const { return sockfd; }

  int getPort() const { return ntohs(address.sin_port); }
  const char* getIP() const { return inet_ntoa(address.sin_addr); }
  sockaddr_in getAddr() const { return address; }
  bool keepAlice() const { return isLinger; }

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

  bool ProcessWrite(HttpCode);
  bool addResponse(const char* format, ...);
  bool addStatusLine(int status, const char* title);
  bool addHeaders(int contentLength);
  bool addContent(const char* content);
  bool addContentLength(int contentLength);
  bool addLinger();
  bool addBlankLine();
  bool addContentType();
  void unmap();

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
  long readIdx;  // the end of readBuf
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

  int bytesHaveSend;
  int bytesToSend;
  struct stat fileStat;
  char* fileAddress;
  struct iovec iv[2];

  int ivCount;
  int cgi;  //是否启用的POST
  char realFile[FILENAME_LEN];

  std::mutex mx;
  MYSQL* mysql;
};

#endif