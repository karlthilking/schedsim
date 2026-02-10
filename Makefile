CXXC=g++
CXXFLAGS=-std=c++20 -g3 -O0 -Wall -Werror -Wextra
PROGRAMS=src/freebsd.cpp src/linux.cpp src/generic.cpp

all: $(PROGRAMS)

freebsd: freebsd.cpp
	$(CXXC) $(CXXFLAGS) -o $@ $<

linux: linux.cpp
	$(CXXC) $(CXXFLAGS) -o $@ $<

generic: generic.cpp
	$(CXXC) $(CXXFLAGS) -o $@ $<

release:
	$(eval CXXFLAGS:=-std=c++20 -03 -Wall -Werror -Wextra)
	$(PROGRAMS)

clean:
	rm -f *.o $(PROGRAMS)

