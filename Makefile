SRCS := $(wildcard **/*.cpp *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:%.o=%.d)
TARGETS  := main
TESTS := test/letter test/str

CPPFLAGS = -MMD -Wfatal-errors -std=c++20 -I. -Wall

all: $(TARGETS)

test: $(TESTS)

%: %.o
	g++ $(CPPFLAGS) $< -o $@

%: %.cpp
	g++ $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf main $(OBJS) $(DEPS) $(TARGETS) $(TESTS)

foo:
	echo $(DEPS)

-include $(DEPS)
