# vim:ts=4:noexpandtab:shiftwidth=4:fileencoding=utf-8:

.PHONY: all clean

all: foo

SRCS := $(shell find . -name '*.cpp')
OBJS := $(SRCS:.cpp=.o)

foo: $(OBJS)
	gcc -o $@ $^ -lstdc++ -pthread

%.o: %.cpp
	gcc -std=c++17 -c -g -pthread -O0 -Wall -Werror -fext-numeric-literals -o $@ $<

clean:
	rm -rf foo *.o
	rm -rf core.*
