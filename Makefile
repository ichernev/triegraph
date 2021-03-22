SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:%.o=%.d)
TARGETS  := $(patsubst %.cpp,%,$(SRCS))
TESTS := $(patsubst %.cpp,%,$(wildcard test/*.cpp))

SHORT := -Wfatal-errors
CPPFLAGS = -MMD $(SHORT) -std=c++20 -Isrc -Wall -O2

.PHONY: all
all: $(TARGETS)

.PHONY: tests
tests: $(TESTS)

.PHONY: test
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
