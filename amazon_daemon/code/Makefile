GCC=g++
CPPFLAGS=-pedantic -Wunused -std=gnu++11 -Wall -Werror -O3
DBGCPPFLAGS= -pedantic -std=gnu++11 -Wall -Werror -Wunused -ggdb3
SRCS=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(SRCS))
DBGOBJS=$(patsubst %.cpp,%.dbg.o,$(SRCS))

.PHONY: clean depend all clobber

all: server
server: $(OBJS)
	$(GCC) -o $@ $(OBJS) amazon.pb.cc ups.pb.cc uacmt.pb.cc -lpthread -lprotobuf -lpqxx -lpq
%.o:%.cpp
	$(GCC) $(CPPFLAGS) -o $@ -c $<
clean:
	rm -f server *~ *.o
depend:
	makedepend $(SRCS)
clobber:
	rm -f *~ *.o
#DO NOT REMOVE
