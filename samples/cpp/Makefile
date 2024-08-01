# Define the compiler
CXX = g++ -g

# List all .cpp and .c source files
CPP_SOURCES := $(wildcard *.cpp)
CPP_EXECUTABLES := $(CPP_SOURCES:.cpp=)

# Default target
all: cache $(CPP_EXECUTABLES)

cache: cache.c
	cc -O0 -g -fno-stack-protector -D_XOPEN_SOURCE=600 -Wno-stringop-overflow -std=c99 cache.c -lm -o cache

# Rule for .cpp files
%: %.cpp
	$(CXX) $< -o $@

# Clean target to remove object files
clean:
	rm -f cache $(CPP_EXECUTABLES)