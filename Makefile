CXXFLAGS=-std=c++20 -g -O3 -Wall -Werror -Wextra # -fsanitize=thread
LDFLAGS=-std=c++20

all: bin/schedsim bin/cpu_task bin/mem_task

bin/schedsim: bin/schedsim.o bin/metrics.o bin/mlfq.o bin/task.o
	g++ $(LDFLAGS) -o $@ bin/schedsim.o bin/metrics.o bin/mlfq.o bin/task.o

bin/schedsim.o: src/schedsim.cpp
	g++ $(CXXFLAGS) -c $< -o $@

bin/metrics.o: src/metrics.cpp include/metrics.hpp
	g++ $(CXXFLAGS) -c $< -o $@

bin/mlfq.o: src/mlfq.cpp include/mlfq.hpp
	g++ $(CXXFLAGS) -c $< -o $@

bin/task.o: src/task.cpp include/task.hpp
	g++ $(CXXFLAGS) -c $< -o $@

# no optimizations
bin/cpu_task: src/cpu_task.cpp
	g++ -o $@ $<

bin/mem_task: src/mem_task.cpp
	g++ -o $@ $<

clean:
	rm -f *.o bin/*

