CXX = g++
CFLAGS = -std=c++17 -O2 -Wall -g

server: ./code/*.cc ./code/Webservice/*.cc ./code/Pool/*.cc
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread -lmysqlclient

clean:
	rm  -r server