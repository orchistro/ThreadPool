# vim:ts=4:noexpandtab:shiftwidth=4:fileencoding=utf-8:

.PHONY: all clean

all: lambda_test async_test

SRCS := $(patsubst ./%,%,$($(shell find . -name '*.cpp')))
DEPS := $(addprefix dep/,$(SRCS:.cpp=.d))

lambda_test: build/lambda_test.o
	$(CXX) -o build/$@ $^ -lstdc++ -pthread

async_test: build/async_test.o
	$(CXX) -o build/$@ $^ -lstdc++ -pthread

$(DEPS): dep/%.d: %.cpp
	@mkdir -p dep
	$(CXX) -std=c++17 -MM -MT $(@:dep/%.d=build/%.o) $< > $@

build/%.o: %.cpp
	@mkdir -p build
	$(CXX) -std=c++17 -c -g -pthread -O0 -Wall -Werror -o $@ $<

ifneq ($(MAKECMDGOALS), clean)
-include $(DEPS)
endif

clean:
	rm -rf dep
	rm -rf build
