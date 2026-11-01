# Makefile for the E-Voting System (Local Simulation)
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude
SRC      := $(wildcard src/*.cpp)
OBJ      := $(SRC:src/%.cpp=build/%.o)
TARGET   := evoting

# Windows/MinGW: link the C++ runtime statically. A dynamically linked .exe needs
# libstdc++-6.dll and libgcc_s_seh-1.dll at run time; when an incompatible
# toolchain's copy sits earlier on PATH (e.g. an old mingw64 install while you
# compiled with ucrt64) the program dies immediately with exit code 127 and no
# message. Static linking makes the .exe self-contained.
ifeq ($(OS),Windows_NT)
  LDFLAGS += -static-libgcc -static-libstdc++ -static
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build:
	mkdir -p build

run: all
	./$(TARGET)

test: all
	bash tests/smoke_test.sh

clean:
	rm -rf build $(TARGET)

.PHONY: all run test clean
