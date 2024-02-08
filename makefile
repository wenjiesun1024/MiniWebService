CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g

server: ./code/*.cc ./code/Webservice/webservice.cc ./code/SQLPool/sqlConnectionPool.cc ./code/Log/log.cc
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient

clean:
	rm  -rf ./log
	rm  -r server