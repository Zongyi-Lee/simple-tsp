
CC := g++

CFLAGS := -Wall -g -O -std=c++11

COMMON=common
SERIALIZATION=serialization
RPC=rpc

INCLUDE_PATH := -Icommon/ -Irpc/ -Iserialization/ 
LIB_PATH := -L serialization/ -L rpc/


COMMON_SRC := $(wildcard $(COMMON)*.cc)
SERIAL_SRC := $(wildcard $(SERIALIZATION)/*.cc)
RPC_SRC := $(wildcard $(RPC)/.*.cc)

all: test_client.cc test_server.cc
	$(CC) $(CFLAGS) $(INCLUDE_PATH) test_client.cc -o test_client $(LIB_PATH) -lrpc -lserial -lpthread
	$(CC) $(CFLAGS) $(INCLUDE_PATH) test_server.cc -o test_server $(LIB_PATH) -lrpc -lserial -lpthread


clean:
	rm -f *.o */*.o 
