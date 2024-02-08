#include "./sqlConnectionPool.h"

#include <cassert>
#include <iostream>

#include "./../Log/log.h"

SqlConnectionPool *SqlConnectionPool::GetInstance() {
  static SqlConnectionPool instance;
  return &instance;
}

void SqlConnectionPool::InitPool(string host, string user, string password,
                                 string dataBaseName, int port, int maxConn) {
  std::lock_guard<std::mutex> lock(mx);
  if (isInit)  // If the pool has been initialized, return directly
  {
    return;
  }

  for (int i = 0; i < maxConn; i++) {
    MYSQL *sql = nullptr;
    sql = mysql_init(sql);
    if (!sql) {
      LOG_FATAL("MySql init error!");
      assert(sql);
    }
    sql = mysql_real_connect(sql, host.c_str(), user.c_str(), password.c_str(),
                             dataBaseName.c_str(), port, nullptr, 0);
    if (!sql) {
      LOG_FATAL("MySql Connect error!");
      assert(sql);
    }
    connectQue.push(sql);
  }

  isInit = true;
}

MYSQL *SqlConnectionPool::GetConnection() {
  MYSQL *sql = nullptr;
  std::unique_lock<std::mutex> lock(mx);

  if (cv.wait_for(lock, std::chrono::seconds(3),
                  [this] { return !connectQue.empty(); })) {
    auto sql = connectQue.front();
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
