
CC := g++

CFLAGS := -Wall -O3 
CXXFLAGS := -std=c++11

COMMON=common
SERIALIZATION=serialization
RPC=rpc

INCLUDE_PATH += COMMON
INCLUDE_PATH += SERIALIZATION
INCLUDE_PATH += RPC


COMMON_SRC := $(wildcard $(COMMON)*.cc)
SERIAL_SRC := $(wildcard $(SERIALIZATION)/*.cc)
RPC_SRC := $(wildcard $(RPC)/.*.cc)

clean:
	rm -f *.o */*.o
