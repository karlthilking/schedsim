CXXFLAGS=-std=c++20 -g -O2 -Wall -Werror -Wextra -fsanitize=thread

all: schedsim

schedsim: src/schedsim.cpp
	g++ $(CXXFLAGS) -o bin/schedsim $<

clean:
	rm -f *.o bin/* build/*

