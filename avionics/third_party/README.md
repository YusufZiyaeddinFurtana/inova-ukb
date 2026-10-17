# Native sensor test dependencies

Unmodified sources are copied from the exact PlatformIO libraries used by the ESP32 target:

- Adafruit BMP3XX **2.1.6**: https://github.com/adafruit/Adafruit_BMP3XX/tree/2.1.6
- Adafruit BusIO **1.17.4** (I²C path): https://github.com/adafruit/Adafruit_BusIO/tree/1.17.4

Original license files and source headers are retained. The bundled Bosch BMP3 sources retain their own notices. `LOCK.json` records hashes for every copied source/license. The sensor test runner verifies both these copies and the corresponding installed PlatformIO files when available. No vendor source is patched.

Native Arduino/Wire compatibility and the unused-SPI failure stub live in `sim/arduino/`. The test uses real Adafruit/Bosch pressure compensation and real BusIO I²C transactions; it does not claim to execute the ESP32 Arduino runtime, FreeRTOS or physical I²C timings.
