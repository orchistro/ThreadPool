# vim:ts=4:noexpandtab:shiftwidth=4:fileencoding=utf-8:

.PHONY: all clean

all: lambda_test async_test

SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:.cpp=.o)
DEPS := $(SRCS:.cpp=.d)

lambda_test: lambda_test.o
	$(CXX) -o $@ $^ -lstdc++ -pthread

async_test: async_test.o
	$(CXX) -o $@ $^ -lstdc++ -pthread

$(DEPS): %.d: %.cpp
	$(CXX) -std=c++17 -MM -MT $(@:%.d=%.o) $< > $@

%.o: %.cpp
	$(CXX) -std=c++17 -c -g -pthread -O0 -Wall -Werror -o $@ $<

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif

clean:
	rm -rf lambda_test *.o *.d
	rm -rf core.*
