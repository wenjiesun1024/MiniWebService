#include "httpConn.h"

#include "./../Log/log.h"

const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form =
    "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form =
    "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form =
    "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form =
    "There was an unusual problem serving the request file.\n";

std::atomic<int> HttpConn::userCount;
TriggerMode HttpConn::connectTriggerMode;
std::mutex mx;
std::map<std::string, std::string> users;

void HttpConn::Init(int fd, int epollfd, const sockaddr_in &addr) {
  sockfd = fd;
  address = addr;
  epollfd = epollfd;

  userCount++;
  LOG_INFO("HttpConn %d init success", sockfd);
}

void HttpConn::Close() {
  close(sockfd);
  userCount--;
  LOG_INFO("HttpConn %d close success", sockfd);
}

bool HttpConn::Read() {
  int bytes_read = 0;

  do {
    // TODO: read data from sockfd
    LOG_INFO("HttpConn %d read success", sockfd);

    bytes_read = recv(sockfd, readBuf + readIdx, READ_BUFFER_SIZE - readIdx, 0);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) break;
      return false;
    } else if (bytes_read == 0) {
      return false;
    }
    readIdx += bytes_read;

  } while (connectTriggerMode == TriggerMode::EDGE);

  return true;
}

bool HttpConn::Process() {
  SqlConnectionRAII mysqlcon(&mysql);

  HttpCode readRet = ProcessRead();
  if (readRet == HttpCode::NO_REQUEST) {
    // modfd(m_epollfd, m_sockfd, EPOLLIN, m_TRIGMode);
    return false;
  }
  bool writeRet = ProcessWrite(readRet);
  if (!writeRet) {
    CloseConn();
  }
  // modfd(m_epollfd, m_sockfd, EPOLLOUT, m_TRIGMode);
  return true;
}

HttpCode HttpConn::ProcessRead() {
  LineStatus line_status = LineStatus::LINE_OK;
  HttpCode ret = HttpCode::NO_REQUEST;
  char *text = nullptr;

  while ((checkState == CheckState::CHECK_STATE_CONTENT &&
          line_status == LineStatus::LINE_OK) ||
         ((line_status = parseLine()) == LineStatus::LINE_OK)) {
    text = getLine();

    LOG_INFO("%s", text);

    startLine = checkedIdx;

    switch (checkState) {
      case CheckState::CHECK_STATE_REQUESTLINE: {
        ret = parseRequestLine(text);
        if (ret == HttpCode::BAD_REQUEST) return HttpCode::BAD_REQUEST;
        break;
      }
      case CheckState::CHECK_STATE_HEADER: {
        ret = parseHeaders(text);
        if (ret == HttpCode::BAD_REQUEST)
          return HttpCode::BAD_REQUEST;
        else if (ret == HttpCode::GET_REQUEST) {
          return doRequest();
        }
        break;
      }
      case CheckState::CHECK_STATE_CONTENT: {
        ret = parseContent(text);
        if (ret == HttpCode::GET_REQUEST) return doRequest();
        line_status = LineStatus::LINE_OPEN;
        break;
      }
      default:
        return HttpCode::INTERNAL_ERROR;
    }
  }
  return HttpCode::NO_REQUEST;
}

// parse the request line, get the method, url, and version
HttpCode HttpConn::parseRequestLine(char *text) {
  url = const_cast<char *>(std::strpbrk(text, " \t"));
  if (!url) {
    return HttpCode::BAD_REQUEST;
  }
  *url++ = '\0';

  const char *methodStr = text;
  if (strcasecmp(methodStr, "GET") == 0)
    method = HttpMethod::GET;
  else if (strcasecmp(methodStr, "POST") == 0) {
    method = HttpMethod::POST;
    cgi = 1;  // TODO: what is cgi?
  } else {
    return HttpCode::BAD_REQUEST;
  }

  url += strspn(url, " \t");
  version = strpbrk(url, " \t");

  if (!version) return HttpCode::BAD_REQUEST;
  *version++ = '\0';

  version += strspn(version, " \t");

  if (strcasecmp(version, "HTTP/1.1") != 0) return HttpCode::BAD_REQUEST;
  if (strncasecmp(url, "http://", 7) == 0) {
    url += 7;
    url = strchr(url, '/');
  }

  if (strncasecmp(url, "https://", 8) == 0) {
    url += 8;
    url = strchr(url, '/');
  }

  if (!url || url[0] != '/') return HttpCode::BAD_REQUEST;
  //当url为/时，显示判断界面
  if (strlen(url) == 1) strcat(url, "judge.html");

  checkState = CheckState::CHECK_STATE_HEADER;
  return HttpCode::NO_REQUEST;
}

// parse the request headers, get the content length, host, and connection
HttpCode HttpConn::parseHeaders(char *text) {
  if (text[0] == '\0') {
    if (contentLength != 0) {
      checkState = CheckState::CHECK_STATE_CONTENT;
      return HttpCode::NO_REQUEST;
    }
    return HttpCode::GET_REQUEST;
  } else if (strncasecmp(text, "Connection:", 11) == 0) {
    text += 11;
    text += strspn(text, " \t");
    if (strcasecmp(text, "keep-alive") == 0) {
      isLinger = true;
    }
  } else if (strncasecmp(text, "Content-length:", 15) == 0) {
    text += 15;
    text += strspn(text, " \t");
    contentLength = atol(text);
  } else if (strncasecmp(text, "Host:", 5) == 0) {
    text += 5;
    text += strspn(text, " \t");
    host = text;
  } else {
    LOG_INFO("oop!unknow header: %s", text);
  }
  return HttpCode::NO_REQUEST;
}

// get the content from the request
HttpCode HttpConn::parseContent(char *text) {
  if (readIdx >= (contentLength + checkedIdx)) {
    text[contentLength] = '\0';
    // POST请求中最后为输入的用户名和密码
    content = text;
    return HttpCode::GET_REQUEST;
  }
  return HttpCode::NO_REQUEST;
}

// get the line from the read buffer
LineStatus HttpConn::parseLine() {
  char temp;
  for (; checkedIdx < readIdx; ++checkedIdx) {
    temp = readBuf[checkedIdx];
    if (temp == '\r') {
      if ((checkedIdx + 1) == readIdx)
        return LineStatus::LINE_OPEN;
      else if (readBuf[checkedIdx + 1] == '\n') {
        readBuf[checkedIdx++] = '\0';
        readBuf[checkedIdx++] = '\0';
        return LineStatus::LINE_OK;
      }
      return LineStatus::LINE_BAD;
    } else if (temp == '\n') {
      if (checkedIdx > 1 && readBuf[checkedIdx - 1] == '\r') {
        readBuf[checkedIdx - 1] = '\0';
        readBuf[checkedIdx++] = '\0';
        return LineStatus::LINE_OK;
      }
      return LineStatus::LINE_BAD;
    }
  }
  return LineStatus::LINE_OPEN;
}

HttpCode HttpConn::doRequest() {
  char serverPath[200];
  getcwd(serverPath, 200);
  char resource[10] = "/resource";
  char *docResource = (char *)malloc(strlen(serverPath) + strlen(resource) + 1);
  strcpy(docResource, serverPath);
  strcat(docResource, resource);

  strcpy(realFile, docResource);
  int len = strlen(docResource);
  const char *p = strrchr(url, '/');

  //处理cgi
  if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {
    //根据标志判断是登录检测还是注册检测
    char flag = url[1];

    char *urlReal = (char *)malloc(sizeof(char) * 200);
    strcpy(urlReal, "/");
    strcat(urlReal, url + 2);
    strncpy(realFile + len, urlReal, FILENAME_LEN - len - 1);
    free(urlReal);

    //将用户名和密码提取出来
    // user=123&passwd=123
    char name[100], password[100];
    int i;
    for (i = 5; content[i] != '&'; ++i) name[i - 5] = content[i];
    name[i - 5] = '\0';

    int j = 0;
    for (i = i + 10; content[i] != '\0'; ++i, ++j) password[j] = content[i];
    password[j] = '\0';

    if (*(p + 1) == '3') {
      //如果是注册，先检测数据库中是否有重名的
      //没有重名的，进行增加数据
      char *sql_insert = (char *)malloc(sizeof(char) * 200);
      strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
      strcat(sql_insert, "'");
      strcat(sql_insert, name);
      strcat(sql_insert, "', '");
      strcat(sql_insert, password);
      strcat(sql_insert, "')");

      if (users.find(name) == users.end()) {
        {
          std::unique_lock<std::mutex> lock(mx);
          int res = mysql_query(mysql, sql_insert);
          users.emplace(name, password);
          if (!res)
            strcpy(url, "/log.html");
          else
            strcpy(url, "/registerError.html");
        }
      } else
        strcpy(url, "/registerError.html");
    }
    //如果是登录，直接判断
    //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
    else if (*(p + 1) == '2') {
      if (users.find(name) != users.end() && users[name] == password)
        strcpy(url, "/welcome.html");
      else
        strcpy(url, "/logError.html");
    }
  }

  if (*(p + 1) == '0') {
    char *m_url_real = (char *)malloc(sizeof(char) * 200);
    strcpy(m_url_real, "/register.html");
    strncpy(realFile + len, m_url_real, strlen(m_url_real));

    free(m_url_real);
  } else if (*(p + 1) == '1') {
    char *m_url_real = (char *)malloc(sizeof(char) * 200);
    strcpy(m_url_real, "/log.html");
    strncpy(realFile + len, m_url_real, strlen(m_url_real));

    free(m_url_real);
  } else if (*(p + 1) == '5') {
    char *m_url_real = (char *)malloc(sizeof(char) * 200);
    strcpy(m_url_real, "/picture.html");
    strncpy(realFile + len, m_url_real, strlen(m_url_real));

    free(m_url_real);
  } else if (*(p + 1) == '6') {
    char *m_url_real = (char *)malloc(sizeof(char) * 200);
    strcpy(m_url_real, "/video.html");
    strncpy(realFile + len, m_url_real, strlen(m_url_real));

    free(m_url_real);
  } else if (*(p + 1) == '7') {
    char *m_url_real = (char *)malloc(sizeof(char) * 200);
    strcpy(m_url_real, "/fans.html");
    strncpy(realFile + len, m_url_real, strlen(m_url_real));

    free(m_url_real);
  } else {
    strncpy(realFile + len, url, FILENAME_LEN - len - 1);
  }
  if (stat(realFile, &fileStat) < 0) return HttpCode::NO_RESOURCE;

  if (!(fileStat.st_mode & S_IROTH)) return HttpCode::FORBIDDEN_REQUEST;

  if (S_ISDIR(fileStat.st_mode)) return HttpCode::BAD_REQUEST;

  int fd = open(realFile, O_RDONLY);
  fileAddress =
      (char *)mmap(0, fileStat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);
  return HttpCode::FILE_REQUEST;
}

void HttpConn::unmap() {
  if (fileAddress) {
    munmap(fileAddress, fileStat.st_size);
    fileAddress = nullptr;
  }
}

bool HttpConn::Write(int &writeErrno) {
  int temp = 0;

  if (bytesToSend == 0) {
    Reset();
    return true;
  }

  while (1) {
    temp = writev(sockfd, iv, ivCount);

    if (temp < 0) {
      if (errno == EAGAIN) {
        writeErrno = EAGAIN;
        return true;
      }
      unmap();
      return false;
    }

    bytesHaveSend += temp;
    bytesToSend -= temp;

    if (bytesHaveSend >= iv[0].iov_len) {
      iv[0].iov_len = 0;
      iv[1].iov_base = fileAddress + (bytesHaveSend - writeIdx);
      iv[1].iov_len = bytesToSend;
    } else {
      iv[0].iov_base = writeBuf + bytesHaveSend;
      iv[0].iov_len = iv[0].iov_len - bytesHaveSend;
    }

    if (bytesToSend <= 0) {
      unmap();

      if (isLinger) {
        Reset();
      }
      return true;
    }
  }
}

bool HttpConn::addResponse(const char *format, ...) {
  if (writeIdx >= WRITE_BUFFER_SIZE) return false;
  va_list arg_list;
  va_start(arg_list, format);
  int len = vsnprintf(writeBuf + writeIdx, WRITE_BUFFER_SIZE - 1 - writeIdx,
                      format, arg_list);
  if (len >= (WRITE_BUFFER_SIZE - 1 - writeIdx)) {
    va_end(arg_list);
    return false;
  }
  writeIdx += len;
  va_end(arg_list);

  LOG_INFO("request:%s", writeBuf);

  return true;
}

bool HttpConn::addStatusLine(int status, const char *title) {
  return addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool HttpConn::addHeaders(int content_len) {
  return addContentLength(content_len) && addLinger() && addBlankLine();
}

bool HttpConn::addContentLength(int content_len) {
  return addResponse("Content-Length:%d\r\n", content_len);
}

bool HttpConn::addContentType() {
  return addResponse("Content-Type:%s\r\n", "text/html");
}

bool HttpConn::addLinger() {
  return addResponse("Connection:%s\r\n", isLinger ? "keep-alive" : "close");
}

bool HttpConn::addBlankLine() { return addResponse("%s", "\r\n"); }

bool HttpConn::addContent(const char *content) {
  return addResponse("%s", content);
}

bool HttpConn::ProcessWrite(HttpCode ret) {
  switch (ret) {
    case HttpCode::INTERNAL_ERROR: {
      addStatusLine(500, error_500_title);
      addHeaders(strlen(error_500_form));
      if (!addContent(error_500_form)) return false;
      break;
    }
    case HttpCode::BAD_REQUEST: {
      addStatusLine(404, error_404_title);
      addHeaders(strlen(error_404_form));
      if (!addContent(error_404_form)) return false;
      break;
    }
    case HttpCode::FORBIDDEN_REQUEST: {
      addStatusLine(403, error_403_title);
      addHeaders(strlen(error_403_form));
      if (!addContent(error_403_form)) return false;
      break;
    }
    case HttpCode::FILE_REQUEST: {
      addStatusLine(200, ok_200_title);
      if (fileStat.st_size != 0) {
        addHeaders(fileStat.st_size);
        iv[0].iov_base = writeBuf;
        iv[0].iov_len = writeIdx;
        iv[1].iov_base = fileAddress;
        iv[1].iov_len = fileStat.st_size;
        ivCount = 2;

        bytesToSend = writeIdx + fileStat.st_size;
        return true;
      } else {
        const char *okStr = "<html><body></body></html>";
        addHeaders(strlen(okStr));
        if (!addContent(okStr)) return false;
      }
    }
    default:
      return false;
  }
  iv[0].iov_base = writeBuf;
  iv[0].iov_len = writeIdx;
  ivCount = 1;
  bytesToSend = writeIdx;
  return true;
}

void InitmysqlResult() {
  MYSQL *mysql = nullptr;
  SqlConnectionRAII mysqlcon(&mysql);

  if (mysql_query(mysql, "SELECT username, passwd FROM user")) {
    LOG_ERROR("SELECT error:%s\n", mysql_error(mysql));
  }

  MYSQL_RES *result = mysql_store_result(mysql);

  int num_fields = mysql_num_fields(result);

  MYSQL_FIELD *fields = mysql_fetch_fields(result);

  while (MYSQL_ROW row = mysql_fetch_row(result)) {
    users[std::string(row[0])] = std::string(row[1]);
  }
}

void HttpConn::CloseConn(bool realClose) {
  if (realClose && (sockfd != -1)) {
    LOG_INFO("close fd %d", sockfd);
    epoll_ctl(epollFd, EPOLL_CTL_DEL, sockfd, 0);
    close(sockfd);
    sockfd = -1;
    userCount--;
  }
}

void HttpConn::Reset() {
  url = nullptr;
  // realFile = nullptr;
  cgi = 0;
  contentLength = 0;
  bytesHaveSend = 0;
  bytesToSend = 0;
  checkState = CheckState::CHECK_STATE_REQUESTLINE;
  method = HttpMethod::GET;
  url = nullptr;
  version = nullptr;
  host = nullptr;
  isLinger = false;
  content.clear();
  startLine = 0;
  checkedIdx = 0;
  readIdx = 0;
  writeIdx = 0;
  unmap();
}