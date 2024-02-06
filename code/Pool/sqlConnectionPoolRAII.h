#ifndef MINI_WEB_SERVICE_SQLCONNECTIONPOOL_RAII_H
#define MINI_WEB_SERVICE_SQLCONNECTIONPOOL_RAII_H

#include "./sqlConnectionPool.h"
class SqlConnectionRAII
{
public:
    SqlConnectionRAII(MYSQL **sql) : sqlConnectionPool(SqlConnectionPool::GetInstance()),
                                     sqlConnection(sqlConnectionPool->GetConnection())
    {
        *sql = sqlConnection;
    }

    ~SqlConnectionRAII()
    {
        sqlConnectionPool->ReleaseConnection(sqlConnection);
    }

private:
    MYSQL *sqlConnection;
    SqlConnectionPool *sqlConnectionPool;
};

#endif