#ifndef MINI_WEB_SERVICE_SQLCONNECTIONPOOL_RAII_H
#define MINI_WEB_SERVICE_SQLCONNECTIONPOOL_RAII_H

#include "./sqlConnectionPool.h"

// FIXME: handle: if SqlConnectionPool has been destroyed

class SqlConnectionRAII {
 public:
  SqlConnectionRAII(MYSQL **sql)
      : sqlConnectionPool(SqlConnectionPool::GetInstance()),
        sqlConnection(sqlConnectionPool->GetConnection()) {
    *sql = sqlConnection;
  }

  ~SqlConnectionRAII() { sqlConnectionPool->ReleaseConnection(sqlConnection); }

 private:
  SqlConnectionPool *sqlConnectionPool;
  MYSQL *sqlConnection;
};

#endif