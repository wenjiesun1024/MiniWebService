CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g

all: ./code/*.cc ./code/Webservice/webservice.cc ./code/SQLPool/sqlConnectionPool.cc ./code/Log/log.cc ./code/Timer/timer.cc ./code/Http/httpConn.cc ./code/Webservice/epoller.cc
	rm -rf ./log
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient
clean:
	rm  -rf ./log
	rm  -r server