#ifndef MINI_WEB_SERVICE_SQLCONNECTIONPOOL_H
#define MINI_WEB_SERVICE_SQLCONNECTIONPOOL_H

#include <mysql/mysql.h>

#include <condition_variable>
#include <queue>
#include <string>

using std::string;

class SqlConnectionPool {
  friend class SqlConnectionRAII;

 public:
  static SqlConnectionPool *GetInstance();  // Singleton pattern

  void InitPool(string host, string user, string password, string dataBaseName,
                int port, int maxConn);
  // TODO: void DestoryPool();

 private:
  MYSQL *GetConnection();
  void ReleaseConnection(MYSQL *sql);

 private:
  SqlConnectionPool() = default;
  ~SqlConnectionPool() = default;
  SqlConnectionPool(const SqlConnectionPool &) = delete;
  SqlConnectionPool &operator=(const SqlConnectionPool &) = delete;

 private:
  std::mutex mx;
  std::condition_variable cv;
  bool isInit = false;

  std::queue<MYSQL *> connectQue;
};

#endif  // MINI_WEB_SERVICE_SQLCONNECTIONPOOL_H