#include "./sqlConnectionPool.h"

#include <cassert>
#include <iostream>

SqlConnectionPool *SqlConnectionPool::GetInstance() {
  static SqlConnectionPool instance;
  return &instance;
}

void SqlConnectionPool::InitPool(string host, string user, string password,
                                 string dataBaseName, int port, int maxConn) {
  std::lock_guard<std::mutex> lock(mx);

  // If the pool has been initialized, return directly
  if (isInit) {
    return;
  }

  for (int i = 0; i < maxConn; i++) {
    MYSQL *sql = mysql_init(nullptr);
    if (!sql) {
      LOG_FATAL("MySql init error!");
    }
    sql = mysql_real_connect(sql, host.c_str(), user.c_str(), password.c_str(),
                             dataBaseName.c_str(), port, nullptr, 0);
    if (!sql) {
      LOG_FATAL("MySql Connect error!");
    }
    connectQue.push(sql);
  }

  isInit = true;
}

// TODO: only destory the connection in the connectQue, not all the connections.
void SqlConnectionPool::DestoryPool() {
  std::unique_lock<std::mutex> lock(mx);
  while (!connectQue.empty()) {
    auto spl = connectQue.front();
    connectQue.pop();
    mysql_close(spl);
  }
}

MYSQL *SqlConnectionPool::GetConnection() {
  MYSQL *sql = nullptr;
  std::unique_lock<std::mutex> lock(mx);

  if (cv.wait_for(lock, std::chrono::seconds(3),
                  [this] { return !connectQue.empty(); })) {
    sql = connectQue.front();
    connectQue.pop();
  }
  return sql;
}

void SqlConnectionPool::ReleaseConnection(MYSQL *sql) {
  if (!sql) {
    return;
  }
  std::lock_guard<std::mutex> lock(mx);
  connectQue.push(sql);
  cv.notify_one();
}