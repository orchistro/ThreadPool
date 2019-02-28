# vim:ts=4:noexpandtab:shiftwidth=4:fileencoding=utf-8:

.PHONY: all clean

all: foo

SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

foo: $(OBJS)
	$(CXX) -o $@ $^ -lstdc++ -pthread

$(DEPS): %.d: %.cpp
	$(CXX) -std=c++17 -MM -MT $(@:%.d=%.o) $< > $@

%.o: %.cpp
	$(CXX) -std=c++17 -c -g -pthread -O0 -Wall -Werror -o $@ $<

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif

clean:
	rm -rf foo *.o *.d
	rm -rf core.*
