#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#define ARDUINO 10819
namespace sim {inline uint64_t clock_us=0;}
inline unsigned long micros(){return static_cast<unsigned long>(sim::clock_us);}
inline unsigned long millis(){return static_cast<unsigned long>(sim::clock_us/1000);}
inline void delay(unsigned long ms){sim::clock_us+=uint64_t(ms)*1000;}
inline void delayMicroseconds(unsigned int us){sim::clock_us+=us;}
