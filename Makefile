CXX=g++
CXXFLAGS=-std=c++1z -lstdc++fs -Wall -pthread -lfmt -lboost_system -O3
CXX_INCLUDE_DIRS:=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I, $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))

all: server view.cgi insert.cgi viewstatic.cgi

%: %.cpp
	$(CXX) $< -o $@ $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)

clean:
	rm -f server *.cgi