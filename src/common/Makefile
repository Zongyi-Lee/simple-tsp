
all: assert.cc thpool.cc
	g++ -Wall -std=c++11 -g -c assert.cc
	g++ -Wall -std=c++11 -g -c thpool.cc



assert.o: assert.cc
	g++ -Wall -std=c++11 -c $<

.PHONY: clean

clean:
	rm -f *.o