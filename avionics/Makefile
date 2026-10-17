CXX ?= c++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Icore
SENSOR_INCLUDES = -Isim/arduino -Isim -Ifirmware -Ithird_party/adafruit_bmp3xx -Ithird_party/adafruit_busio
SENSOR_VENDOR = build/bmp3.o build/adafruit_bmp3xx.o build/adafruit_i2c.o
.PHONY: all test clean
all: build/replay build/core_tests build/ek7_tests build/ek7_replay build/sensor_tests build/sensor_replay
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
build/bmp3.o: third_party/adafruit_bmp3xx/bmp3.c | build
	$(CC) -std=c99 -O2 -Ithird_party/adafruit_bmp3xx -c $< -o $@
build/adafruit_bmp3xx.o: third_party/adafruit_bmp3xx/Adafruit_BMP3XX.cpp sim/arduino/Arduino.h sim/arduino/Wire.h | build
	$(CXX) -std=c++17 -O2 $(SENSOR_INCLUDES) -c $< -o $@
build/adafruit_i2c.o: third_party/adafruit_busio/Adafruit_I2CDevice.cpp sim/arduino/Arduino.h sim/arduino/Wire.h | build
	$(CXX) -std=c++17 -O2 $(SENSOR_INCLUDES) -c $< -o $@
build/sensor_tests: tests/sensor_tests.cpp sim/sensors.hpp sim/sensor_rig.hpp firmware/mpu9255.hpp firmware/bmp388.hpp $(SENSOR_VENDOR) | build
	$(CXX) $(CXXFLAGS) $(SENSOR_INCLUDES) $< $(SENSOR_VENDOR) -o $@
build/sensor_replay: tools/sensor_replay.cpp sim/sensors.hpp sim/sensor_rig.hpp firmware/mpu9255.hpp firmware/bmp388.hpp core/flight.hpp core/ek7.hpp $(SENSOR_VENDOR) | build
	$(CXX) $(CXXFLAGS) $(SENSOR_INCLUDES) $< $(SENSOR_VENDOR) -o $@
test:
	python3 tools/run_sil.py
	python3 tools/run_ek7.py
	python3 tools/run_sensors.py
	python3 tools/check_ek7_traceability.py
clean:
	rm -rf build
