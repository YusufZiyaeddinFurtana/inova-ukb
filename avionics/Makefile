CXX ?= c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Icore
.PHONY: all test clean
all: build/replay build/core_tests build/ek7_tests build/ek7_replay
build:
	mkdir -p build
build/replay: tools/replay.cpp core/flight.hpp core/telemetry.hpp | build
	$(CXX) $(CXXFLAGS) tools/replay.cpp -o $@
build/core_tests: tests/core_tests.cpp core/flight.hpp core/telemetry.hpp | build
	$(CXX) $(CXXFLAGS) tests/core_tests.cpp -o $@
build/ek7_tests: tests/ek7_tests.cpp core/ek7.hpp core/flight.hpp | build
	$(CXX) $(CXXFLAGS) tests/ek7_tests.cpp -o $@
build/ek7_replay: tools/ek7_replay.cpp core/ek7.hpp core/flight.hpp | build
	$(CXX) $(CXXFLAGS) tools/ek7_replay.cpp -o $@
test:
	python3 tools/run_sil.py
	python3 tools/run_ek7.py
	python3 tools/check_ek7_traceability.py
clean:
	rm -rf build
