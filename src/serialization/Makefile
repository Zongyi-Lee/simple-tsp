
CC=g++
CFLAGS := -Wall -std=c++11

INCLUDE_PATH := ../common
SRCS := $(wildcard *.cc)
OBJS := ${patsubst %.cc, %.o, $(SRCS)}

$(OBJS) : $(SRCS)
	$(CC) $(CFLAGS) -g -c $(SRCS)

libserial.a: $(OBJS) ../common/assert.o
	ar cr $@ $^

test: test.o xmlutil.o xmldata.o ../common/assert.o
	$(CC) $(CFLAGS) $^ -g -o $@

.PHONY: clean

clean:
	rm -f *.o *.a test

