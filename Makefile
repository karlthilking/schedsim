CXXFLAGS=-std=c++20 -g3 -O0

all: schedsim

schedsim: src/schedsim.cpp
	g++ $(CXXFLAGS) -o bin/schedsim $<

clean:
	rm -f *.o bin/* build/*

