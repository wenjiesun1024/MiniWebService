#include <iostream>

#include "sqlConnectionPoolRAII.h"

// g++ *.cc ./../Log/log.cc -std=c++17 -O2 -Wall -g -lpthread -lmysqlclient
int main() {
  Log::GetInstance()->Init(LogWriteMode::Sync, LogLevel::INFO, "./log", 1000,
                           1000, 1000);

  SqlConnectionPool::GetInstance()->InitPool("localhost", "root", "root",
                                             "yourdb", 3306, 8);
  LOG_INFO("MySql init Success");

  // TODO: write test
}