#pragma once
#include "Arduino.h"
#include <stdexcept>
struct SPIClass{};
inline SPIClass SPI;
constexpr int SPI_BITORDER_MSBFIRST=1,SPI_MODE0=0;
// SPI is outside this harness. Fail explicitly if accidentally selected.
class Adafruit_SPIDevice {
public:
    template<class... Args> explicit Adafruit_SPIDevice(Args...){}
    bool begin(){throw std::logic_error("Sensor harness supports I2C only");}
    bool write_then_read(const uint8_t*,size_t,uint8_t*,size_t,uint8_t){throw std::logic_error("Unexpected SPI");}
    bool write(uint8_t*,size_t,const uint8_t*,size_t){throw std::logic_error("Unexpected SPI");}
};
