#pragma once
namespace board {
// Proposed ESP32-WROOM-32 DevKit pinout. Confirm against the eventual PCB.
constexpr int imu_sda=21,imu_scl=22,baro_sda=32,baro_scl=33;
constexpr int spi_sck=18,spi_miso=19,spi_mosi=23,lora_cs=13,lora_rst=14,lora_dio0=26;
constexpr int gps_rx=16,gps_tx=17,arm=27,release=25,indicator=2;
constexpr int ek7_rx=35,ek7_tx=4; // UART1 via fixed 3.3V MAX3232; NEVER direct RS232 to ESP32.
constexpr long radio_hz=433000000;
constexpr int radio_sf=7,radio_bw=125000,radio_cr=5,radio_dbm=10;
}
