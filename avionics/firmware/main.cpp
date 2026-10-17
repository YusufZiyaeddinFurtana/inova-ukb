#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BMP3XX.h>
#include <TinyGPSPlus.h>
#include <LoRa.h>
#include <esp_timer.h>
#include "flight.hpp"
#include "telemetry.hpp"
#include "ek7.hpp"
#include "ek7_profile.hpp"
#include "board.hpp"
#include "mpu9255.hpp"

#ifndef INOVA_PHYSICAL_OUTPUT
#define INOVA_PHYSICAL_OUTPUT 0
#endif
// This package defines NO flight-output build. Electrical release polarity/isolation is unverified.
static_assert(INOVA_PHYSICAL_OUTPUT==0,"Physical release needs a reviewed circuit and hardware tests");
static_assert(configTICK_RATE_HZ>=1000,"EK7 task requires 1ms RTOS scheduling");

MPU9255 imu(Wire);
Adafruit_BMP3XX baro;
TinyGPSPlus gps;
HardwareSerial gps_uart(2);
HardwareSerial test_uart(1);
TwoWire baro_wire(1);
bool imu_ready=false,baro_ready=false,radio_ready=false;
volatile bool transmitting=false;
uint64_t last_tick=0,last_tx=0,tx_started=0;
uint16_t sequence=0;
void onTxDone(){transmitting=false;}
struct BaroReading {double pressure;uint64_t time_us;};
QueueHandle_t baro_queue=nullptr;
QueueHandle_t sensor_queue=nullptr,output_queue=nullptr;
void ek7Task(void*) {
    // Single owner for the flight core and EK-7 session. No shared mutable estimator state.
    inova::ek7::Session session(ek7Profile());
    TickType_t next=xTaskGetTickCount();
    for(;;) {
        session.setArm(digitalRead(board::arm)==LOW);
        for(int n=0;n<256 && test_uart.available();n++)session.receive(test_uart.read(),esp_timer_get_time());
        inova::Sample sample;
        if(xQueueReceive(sensor_queue,&sample,0)==pdTRUE)session.realSample(sample);
        auto frame=session.poll(esp_timer_get_time());
        if(frame.size && test_uart.availableForWrite()>=int(frame.size))test_uart.write(frame.bytes.data(),frame.size);
        auto out=session.output();
        if(session.mode()!=inova::ek7::Mode::Normal)out.health|=0x0400; // Bench/test data in LoRa telemetry.
        xQueueOverwrite(output_queue,&out);
        digitalWrite(board::indicator,out.release_requested?HIGH:LOW);
        digitalWrite(board::release,LOW);
        vTaskDelayUntil(&next,pdMS_TO_TICKS(1));
    }
}
void barometerTask(void*) {
    TickType_t next=xTaskGetTickCount();
    for(;;) {
        if(baro.performReading()) {
            BaroReading reading{baro.pressure,static_cast<uint64_t>(esp_timer_get_time())};
            xQueueOverwrite(baro_queue,&reading);
        }
        vTaskDelayUntil(&next,pdMS_TO_TICKS(20));
    }
}

void setup() {
    pinMode(board::release,OUTPUT);digitalWrite(board::release,LOW);
    pinMode(board::indicator,OUTPUT);digitalWrite(board::indicator,LOW);
    pinMode(board::arm,INPUT_PULLUP);
    Serial.begin(115200);
    test_uart.setRxBufferSize(1024);
    test_uart.begin(115200,SERIAL_8N1,board::ek7_rx,board::ek7_tx);
    Wire.begin(board::imu_sda,board::imu_scl,400000);Wire.setTimeOut(5);
    baro_wire.begin(board::baro_sda,board::baro_scl,400000);baro_wire.setTimeOut(5);
    imu_ready=imu.begin();
    baro_ready=baro.begin_I2C(0x77,&baro_wire);
    if(!baro_ready)baro_ready=baro.begin_I2C(0x76,&baro_wire);
    if(baro_ready) {
        baro_ready=baro.chipID()==0x50 && baro.setPressureOversampling(BMP3_OVERSAMPLING_2X) &&
            baro.setTemperatureOversampling(BMP3_NO_OVERSAMPLING) &&
            baro.setIIRFilterCoeff(BMP3_IIR_FILTER_DISABLE) && baro.setOutputDataRate(BMP3_ODR_50_HZ);
    }
    sensor_queue=xQueueCreate(1,sizeof(inova::Sample));output_queue=xQueueCreate(1,sizeof(inova::Output));
    if(!sensor_queue || !output_queue || xTaskCreatePinnedToCore(ek7Task,"ek7",6144,nullptr,3,nullptr,0)!=pdPASS) {
        // Allocation failure cannot leave an apparently operational flight controller.
        for(;;){digitalWrite(board::release,LOW);delay(1000);}
    }
    gps_uart.setRxBufferSize(1024);gps_uart.begin(9600,SERIAL_8N1,board::gps_rx,board::gps_tx);
    SPI.begin(board::spi_sck,board::spi_miso,board::spi_mosi,board::lora_cs);
    LoRa.setPins(board::lora_cs,board::lora_rst,board::lora_dio0);
    radio_ready=LoRa.begin(board::radio_hz);
    if(radio_ready) {
        LoRa.setSpreadingFactor(board::radio_sf);LoRa.setSignalBandwidth(board::radio_bw);
        LoRa.setCodingRate4(board::radio_cr);LoRa.setPreambleLength(8);LoRa.setSyncWord(0x12);
        LoRa.enableCrc();LoRa.setTxPower(board::radio_dbm);LoRa.onTxDone(onTxDone);
    }
    if(baro_ready) {
        baro_queue=xQueueCreate(1,sizeof(BaroReading));
        if(!baro_queue || xTaskCreatePinnedToCore(barometerTask,"barometer",4096,nullptr,1,nullptr,0)!=pdPASS)baro_ready=false;
    }
}

void loop() {
    for(int n=0;n<128&&gps_uart.available();n++)gps.encode(gps_uart.read());
    uint64_t now=esp_timer_get_time();
    if(now-last_tick<10000) {delay(1);return;}
    last_tick=now;
    inova::Sample s;s.time_s=now/1e6;s.arm=digitalRead(board::arm)==LOW;
    s.imu_new=imu_ready&&imu.read(s.accel_m_s2,s.gyro_rad_s);
    BaroReading reading{};
    if(baro_ready && xQueueReceive(baro_queue,&reading,0)==pdTRUE) {
        uint64_t age=static_cast<uint64_t>(esp_timer_get_time())-reading.time_us;
        if(age<=100000) {s.pressure_pa=reading.pressure;s.baro_new=true;}
    }
    s.time_s=esp_timer_get_time()/1e6;
    xQueueOverwrite(sensor_queue,&s);
    static inova::Output out;
    xQueueReceive(output_queue,&out,0);
    now=esp_timer_get_time();
    if(transmitting && now-tx_started>250000) {LoRa.sleep();transmitting=false;radio_ready=false;}
    if(now-last_tx>=200000) {
        last_tx=now;
        inova::Telemetry t;t.sequence=sequence++;t.uptime_ms=millis();t.flight=out;
        t.gps_valid=gps.location.isValid()&&gps.location.age()<2000;
        if(t.gps_valid){t.latitude_e7=std::lround(gps.location.lat()*1e7);t.longitude_e7=std::lround(gps.location.lng()*1e7);}
        t.health=0x0100; // Battery voltage unmeasured: no divider circuit supplied.
        if(!radio_ready)t.health|=0x0200;
        auto packet=inova::encode(t);
        if(Serial.availableForWrite()>=int(packet.size()))Serial.write(packet.data(),packet.size());
        if(radio_ready&&!transmitting&&LoRa.beginPacket()) {
            LoRa.write(packet.data(),packet.size());transmitting=true;tx_started=now;
            if(!LoRa.endPacket(true)) {transmitting=false;radio_ready=false;}
        }
    }
}
