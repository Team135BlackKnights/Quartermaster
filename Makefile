CXX=g++
CXXFLAGS=-std=c++17 -Wall -Werror
LDFLAGS=`mysql_config --cflags --libs` -lmysqlclient

parts: parts.cpp data_types.h util.h data_types.cpp util.cpp queries.cpp queries.h auth.h auth.cpp
	$(CXX) $(CXXFLAGS) parts.cpp data_types.cpp util.cpp queries.cpp auth.cpp $(LDFLAGS) -o parts

.PHONY: clean
clean:
	rm -f parts
