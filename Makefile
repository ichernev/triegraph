SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %.cpp,%.o,$(SRCS))
DEPS := $(OBJS:%.o=%.d)

CPPFLAGS = -MMD

-include $(DEPS)

all: main

main: $(OBJS)

main: $(OBJS)
	g++ $(CPPFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -rf main $(OBJS) $(DEPS)
