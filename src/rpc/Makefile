
CC := g++
CFLAGS := -Wall -std=c++11 -g

INCLUDE_PATH := -I../common/

HEADERS := $(wildcard *.h)
SRCS := $(wildcard *.cc)
OBJS := ${patsubst %.cc, %.o, $(SRCS)}

$(OBJS) : $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDE_PATH) -c $(SRCS)


librpc.a: $(OBJS) ../common/thpool.o $(HEADERS)
	ar cr $@ $^

.PHONY: clean

clean:
	rm -f librpc.a *.o


