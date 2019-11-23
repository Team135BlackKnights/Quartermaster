CXX=g++
CXXFLAGS=-std=c++17 -Wall -Werror
LDFLAGS=`mysql_config --cflags --libs` -lmysqlclient

DEPS:=auth.o data_types.o parts.o queries.o util.o

all: parts

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

parts: $(DEPS)
	$(CXX) $(DEPS) $(LDFLAGS) -o parts

.PHONY: clean
clean:
	rm -f $(DEPS) parts
