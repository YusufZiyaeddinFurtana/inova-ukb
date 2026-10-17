CXX ?= c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Icore
.PHONY: all test clean
all: build/replay build/core_tests
build:
	mkdir -p build
build/replay: tools/replay.cpp core/flight.hpp core/telemetry.hpp | build
	$(CXX) $(CXXFLAGS) tools/replay.cpp -o $@
build/core_tests: tests/core_tests.cpp core/flight.hpp core/telemetry.hpp | build
	$(CXX) $(CXXFLAGS) tests/core_tests.cpp -o $@
test:
	python3 tools/run_sil.py
clean:
	rm -rf build
