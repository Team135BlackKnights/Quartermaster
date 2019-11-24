CXX=g++
CXXFLAGS=-std=c++17 -Wall -Werror -g
LDFLAGS=`mysql_config --cflags --libs` -lmysqlclient

DEPS:=auth.o data_types.o parts.o queries.o util.o export.o home.o

all: parts

%.o: %.cpp *.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

parts: $(DEPS) *.h
	$(CXX) $(DEPS) $(LDFLAGS) -o parts

.PHONY: clean
clean:
	rm -f $(DEPS) parts
