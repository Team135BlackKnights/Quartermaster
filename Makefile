#Note:
#If the optimization is turned up on g++ 7 it print warnings
#which go away if g++ 9 is used.

CXXFLAGS=-std=c++17 -Wall -Werror -g
LDFLAGS=`mysql_config --cflags --libs` -lmysqlclient

DEPS:=auth.o data_types.o parts.o queries.o \
	util.o export.o home.o subsystems.o subsystem.o part.o \
	meeting.o order.o progress.o

all: parts asm.png part.png

asm.png:
	dot -Tpng asm.dot > asm.png

part.png:
	dot -Tpng part.dot > part.png

%.o: %.cpp *.h
	$(CXX) -c -o $@ $< $(CXXFLAGS)

parts: $(DEPS) *.h
	$(CXX) $(DEPS) $(LDFLAGS) -o parts

install: all
	cp parts asm.png part.png $(INSTALL_DIR)

.PHONY: clean
clean:
	rm -f $(DEPS) parts asm.png part.png
