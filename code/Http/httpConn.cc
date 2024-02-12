#include "httpConn.h"

#include "./../Log/log.h"

std::atomic<int> HttpConn::userCount;
TriggerMode HttpConn::connectTriggerMode;

void HttpConn::Init(int fd, const sockaddr_in &addr) {
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

bool HttpConn::Write() {
  // TODO: write data to sockfd
  return true;
}

bool HttpConn::Process() {
  // TODO: process data
  ProcessRead();
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
    // cgi = 1; TODO: what is cgi?
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
  std::cout << content << " " << url << " " << host << " "
            << " " << version << std::endl;

  // strcpy(m_real_file, doc_root);
  // int len = strlen(doc_root);
  // // printf("m_url:%s\n", m_url);
  // const char *p = strrchr(url, '/');

  // //处理cgi
  // if (cgi == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {
  //   //根据标志判断是登录检测还是注册检测
  //   char flag = url[1];

  //   char *m_url_real = (char *)malloc(sizeof(char) * 200);
  //   strcpy(m_url_real, "/");
  //   strcat(m_url_real, url + 2);
  //   strncpy(m_real_file + len, m_url_real, FILENAME_LEN - len - 1);
  //   free(m_url_real);

  //   //将用户名和密码提取出来
  //   // user=123&passwd=123
  //   char name[100], password[100];
  //   int i;
  //   for (i = 5; content[i] != '&'; ++i) name[i - 5] = content[i];
  //   name[i - 5] = '\0';

  //   int j = 0;
  //   for (i = i + 10; content[i] != '\0'; ++i, ++j) password[j] = content[i];
  //   password[j] = '\0';

  //   if (*(p + 1) == '3') {
  //     //如果是注册，先检测数据库中是否有重名的
  //     //没有重名的，进行增加数据
  //     char *sql_insert = (char *)malloc(sizeof(char) * 200);
  //     strcpy(sql_insert, "INSERT INTO user(username, passwd) VALUES(");
  //     strcat(sql_insert, "'");
  //     strcat(sql_insert, name);
  //     strcat(sql_insert, "', '");
  //     strcat(sql_insert, password);
  //     strcat(sql_insert, "')");

  //     if (users.find(name) == users.end()) {
  //       m_lock.lock();
  //       int res = mysql_query(mysql, sql_insert);
  //       users.insert(pair<string, string>(name, password));
  //       m_lock.unlock();

  //       if (!res)
  //         strcpy(url, "/log.html");
  //       else
  //         strcpy(url, "/registerError.html");
  //     } else
  //       strcpy(url, "/registerError.html");
  //   }
  //   //如果是登录，直接判断
  //   //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
  //   else if (*(p + 1) == '2') {
  //     if (users.find(name) != users.end() && users[name] == password)
  //       strcpy(url, "/welcome.html");
  //     else
  //       strcpy(url, "/logError.html");
  //   }
  // }

  // if (*(p + 1) == '0') {
  //   char *m_url_real = (char *)malloc(sizeof(char) * 200);
  //   strcpy(m_url_real, "/register.html");
  //   strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

  //   free(m_url_real);
  // } else if (*(p + 1) == '1') {
  //   char *m_url_real = (char *)malloc(sizeof(char) * 200);
  //   strcpy(m_url_real, "/log.html");
  //   strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

  //   free(m_url_real);
  // } else if (*(p + 1) == '5') {
  //   char *m_url_real = (char *)malloc(sizeof(char) * 200);
  //   strcpy(m_url_real, "/picture.html");
  //   strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

  //   free(m_url_real);
  // } else if (*(p + 1) == '6') {
  //   char *m_url_real = (char *)malloc(sizeof(char) * 200);
  //   strcpy(m_url_real, "/video.html");
  //   strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

  //   free(m_url_real);
  // } else if (*(p + 1) == '7') {
  //   char *m_url_real = (char *)malloc(sizeof(char) * 200);
  //   strcpy(m_url_real, "/fans.html");
  //   strncpy(m_real_file + len, m_url_real, strlen(m_url_real));

  //   free(m_url_real);
  // } else
  //   strncpy(m_real_file + len, url, FILENAME_LEN - len - 1);

  // if (stat(m_real_file, &m_file_stat) < 0) return HttpCode::NO_RESOURCE;

  // if (!(m_file_stat.st_mode & S_IROTH)) return HttpCode::FORBIDDEN_REQUEST;

  // if (S_ISDIR(m_file_stat.st_mode)) return HttpCode::BAD_REQUEST;

  // int fd = open(m_real_file, O_RDONLY);
  // m_file_address =
  //     (char *)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  // close(fd);
  // return HttpCode::FILE_REQUEST;
}