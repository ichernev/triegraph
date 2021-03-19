SRCS := $(wildcard **/*.cpp *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:%.o=%.d)
TARGETS  := $(patsubst %.cpp,%,$(wildcard *.cpp))
TESTS := $(patsubst %.cpp,%,$(wildcard test/*.cpp))

SHORT := -Wfatal-errors
CPPFLAGS = -MMD $(SHORT) -std=c++20 -I. -Wall -O2

all: $(TARGETS)

tests: $(TESTS)

test: $(TESTS)
	for t in $(TESTS); do echo ======= $$t =======; ./$$t; done 2>&1 | awk ' BEGIN { RED = "\033[1;31m"; GREEN = "\033[1;32m"; COLEND = "\033[0m" } /=======/ { printf GREEN; } /Assertion/ { printf RED; } // { print $$0 COLEND; } '

%: %.o
	g++ $(CPPFLAGS) $< -o $@

%: %.cpp
	g++ $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf main $(OBJS) $(DEPS) $(TARGETS) $(TESTS)

-include $(DEPS)
