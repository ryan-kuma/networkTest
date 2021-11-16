.PHONY: all clean
#CC=g++
#BIN=iptest chatserver
#all:$(BIN)
#%.o:%.cpp
#	$(CC) -o $@ $<
#clean:
#	rm -rf *.o $(BIN)


CC=gcc
BIN=hello
CFLAGS=-I/home/ryan/workspace/libevent-2.1.12-stable/include/event2
LIBS= /usr/local/lib/libevent.a -lrt
hello: hello.c 
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
clean:
	rm -f $(BIN)
