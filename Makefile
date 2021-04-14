SRCS := $(wildcard src/*.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
TARGETS  := $(patsubst %.cpp,%,$(SRCS))
TESTS := $(patsubst %.cpp,%,$(wildcard test/*.cpp))
SLOWS := $(patsubst %.cpp,%,$(wildcard slow/*.cpp))
BENCHES := $(patsubst %.cpp,%,$(wildcard benchmark/*.cpp))
DEPS := $(OBJS:%.o=%.d) $(TESTS:%=%.d) $(SLOWS:%=%.d)

SHORT := -Wfatal-errors
CPPFLAGS = -MMD $(SHORT) -std=c++20 -Isrc -Itest -Wall -O2

.PHONY: all
all: $(TARGETS)

.PHONY: tests
tests: $(TESTS)

.PHONY: slow
slow: $(SLOWS)

.PHONY: benchmarks
benchmarks: $(BENCHES)

.PHONY: test
test: $(TESTS)
	for t in $(TESTS); do echo ======= $$t =======; ./$$t; done 2>&1 | awk ' BEGIN { RED = "\033[1;31m"; GREEN = "\033[1;32m"; COLEND = "\033[0m" } /=======/ { printf GREEN; } /Assertion|terminate/ { printf RED; } // { print $$0 COLEND; } '

.PHONY: benchmark
benchmark: $(BENCHES)
	for b in $(BENCHES); do echo ======= $$b =======; ./$$b; done 2>&1 | awk ' BEGIN { RED = "\033[1;31m"; GREEN = "\033[1;32m"; COLEND = "\033[0m" } /=======/ { printf GREEN; } /Assertion|terminate/ { printf RED; } // { print $$0 COLEND; } '

%: %.o
	g++ $(CPPFLAGS) $< -o $@

%: %.cpp
	g++ $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf main $(OBJS) $(DEPS) $(TARGETS) $(TESTS)

-include $(DEPS)
