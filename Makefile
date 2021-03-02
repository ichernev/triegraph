SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:%.o=%.d)

CPPFLAGS = -MMD -Wfatal-errors -std=c++20

all: main

main: $(OBJS)
	g++ $(CPPFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf main $(OBJS) $(DEPS)

-include $(DEPS)
