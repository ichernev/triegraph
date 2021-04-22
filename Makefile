OUTPUT := output

SRCS := $(shell find src -name '*.cpp')
# OBJS := $(patsubst %.cpp,%.o,$(SRCS))
TARGETS  := $(patsubst %.cpp,$(OUTPUT)/%,$(SRCS))
TEST_SRCS := $(shell find test -name '*.cpp')
TEST_TARGETS = $(patsubst %.cpp,$(OUTPUT)/%,$(TEST_SRCS))
# SLOWS := $(patsubst %.cpp,%,$(wildcard slow/*.cpp))
# BENCHES := $(patsubst %.cpp,%,$(wildcard benchmark/*.cpp))
DEPS := $(SRCS:%.cpp=$(OUTPUT)/%.d) $(TEST_SRCS:%.cpp=$(OUTPUT)/%.d)

FORCE :=
DEBUG := 0
ifeq ($(DEBUG), 1)
  OPTIMIZE := -g
else
  OPTIMIZE := -O2
endif
SHORT := -Wfatal-errors
CPPFLAGS := -MMD $(SHORT) -std=c++20 -Isrc -Wall $(OPTIMIZE)
CPPFLAGS_TEST := $(CPPFLAGS) -Itest -I../hash_test/sparsepp

TCOLORS := awk ' BEGIN { RED = "\033[1;31m"; GREEN = "\033[1;32m"; COLEND = "\033[0m" } /TEST MODULE/ { printf GREEN; } /Assertion|terminate/ { printf RED; } // { print $$0 COLEND; } '


.PHONY: all
main: $(TARGETS)

.PHONY: tests
tests: $(TEST_TARGETS)


.PHONY: test
test:
	@if [ -n "$(ONLY)" ]; then \
		make --no-print-directory build run ONLY="$(ONLY)" PREFIX=test; \
	else \
		make --no-print-directory tests run-tests; \
	fi

.PHONY: run-tests
run-tests: $(TEST_TARGETS)
	for t in $^; do ./$$t; done 2>&1 | $(TCOLORS)

# build a bunch of targets specified by ONLY parameter
.PHONY: build
build:
	@for pcs in $(ONLY); do \
		xpcs=$${pcs%.cpp}; \
		[ -n "$(PREFIX)" ] && xpcs=$(PREFIX)/$${xpcs#$(PREFIX)/}; \
		if [ -d "$$xpcs" ]; then \
			find $$xpcs/ -name '*.cpp' | while read t; do \
				[ -n "$(FORCE)" ] && rm ./$(OUTPUT)/$${t%.cpp}; \
				make ./$(OUTPUT)/$${t%.cpp}; \
			done; \
		else \
			[ -n "$(FORCE)" ] && rm ./$(OUTPUT)/$$xpcs; \
			make ./$(OUTPUT)/$$xpcs; \
		fi; \
	done

# run a bunch of targets specified by ONLY parameter
.PHONY: run
run:
	@for pcs in $(ONLY); do \
		xpcs=$${pcs%.cpp}; \
		[ -n "$(PREFIX)" ] && xpcs=$(PREFIX)/$${xpcs#$(PREFIX)/}; \
		if [ -d "$$xpcs" ]; then \
			find $$xpcs/ -name '*.cpp' | while read t; do \
				./$(OUTPUT)/$${t%.cpp} 2>&1 | $(TCOLORS); \
			done; \
		else \
			./$(OUTPUT)/$$xpcs 2>&1 | $(TCOLORS); \
		fi; \
	done

$(OUTPUT)/test/%: test/%.cpp
	@mkdir -p $(OUTPUT)/$(shell dirname $<)
	g++ $(CPPFLAGS_TEST) $< -o $@

$(OUTPUT)/%: %.cpp
	@mkdir -p $(OUTPUT)/$(shell dirname $<)
	g++ $(CPPFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -rf main $(DEPS) $(TARGETS) $(TEST_TARGETS)

-include $(DEPS)
