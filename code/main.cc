#include <iostream>

#include "Webservice/webservice.h"
#include "Pool/sqlConnectionPool.h"

int main()
{
    WebService ws(9006, LogWriteMode::Sync, TriggerMode::LEVEL, 8, 8);

    SqlConnectionPool *sqlPool = SqlConnectionPool::GetInstance();
    sqlPool->InitPool("localhost", "root", "root", "yourdb", 3306, 8);

    return 0;
}