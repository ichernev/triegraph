SRCS := $(wildcard **/*.cpp *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:%.o=%.d)
TARGETS  := $(patsubst %.cpp,%,$(wildcard *.cpp))
TESTS := $(patsubst %.cpp,%,$(wildcard test/*.cpp))

SHORT := -Wfatal-errors
CPPFLAGS = -MMD $(SHORT) -std=c++20 -I. -Wall

all: $(TARGETS)

test: $(TESTS)

%: %.o
	g++ $(CPPFLAGS) $< -o $@

%: %.cpp
	g++ $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf main $(OBJS) $(DEPS) $(TARGETS) $(TESTS)

-include $(DEPS)
