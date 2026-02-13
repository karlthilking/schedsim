CXXC=g++
CXXFLAGS=-std=c++20 -g3 -O0 -Wall -Werror -Wextra

all: schedsim

define build
	$(CXXC) $(CXXFLAGS) -o schedsim schedsim.cpp
	@echo '$(CXXC) $(CXXFLAGS) -o schedsim schedsim.cpp'
endef

schedsim:
	@$(call build)

release:
	$(eval CXXFLAGS:=-std=c++20 -03 -Wall -Werror -Wextra)

clean:
	rm -f *.o schedsim

