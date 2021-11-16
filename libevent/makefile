.PHONY: all clean
CC=gcc
CFLAG=-Wall -g
#BIN=write_fifo read_fifo read hello world time_test
#BIN=http-server
BIN=loopHello
LDLIBS=-levent
all:$(BIN)
%.o:%.c
	$(CC) $(CFLAG) -o $@ $^ $(LDLIBS)
clean:
	rm -rf *.o $(BIN)
