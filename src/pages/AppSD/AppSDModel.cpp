#include "AppSDModel.h"

#include <SPI.h>

using namespace Page;

// Wen: The SD card is not working simultaneously with the LCD screen,
// ONLY under heavy LCD usage, such as high-freq camera refreash.
// ref: https://components.espressif.com/components/espressif/m5stack_core_s3/versions/2.0.1/readme
bool AppSDModel::SDInit() {
    if (init_flag) {
        return true;
    }

    /*
     * UserDemo calls SD.begin(GPIO_NUM_4, SPI, 25MHz) only.
     * That works only if Arduino SPI pins match the board.
     * Our platformio board is esp32-s3-devkitc-1 (SCK=12/MISO=13/MOSI=11),
     * while CoreS3 TF is SCK=36 MOSI=37 MISO=35 CS=4.
     * SD.cpp does SPI.begin() with no args → wrong pins → init fails.
     * Fix: bind SPI to CoreS3 pins via M5 pin table before SD.begin.
     */
    int8_t sclk = M5.getPin(m5::pin_name_t::sd_spi_sclk);
    int8_t miso = M5.getPin(m5::pin_name_t::sd_spi_miso);
    int8_t mosi = M5.getPin(m5::pin_name_t::sd_spi_mosi);
    int8_t cs   = M5.getPin(m5::pin_name_t::sd_spi_cs);
    if (sclk < 0) {
        sclk = 36;
    }
    if (miso < 0) {
        miso = 35;
    }
    if (mosi < 0) {
        mosi = 37;
    }
    if (cs < 0) {
        cs = 4;
    }

    USBSerial.printf("[SD] SPI sclk=%d miso=%d mosi=%d cs=%d\n", sclk, miso,
                     mosi, cs);

    SPI.end();
    SPI.begin(sclk, miso, mosi, cs);

    init_flag = SD.begin((uint8_t)cs, SPI, 25000000);
    USBSerial.printf("[SD] begin -> %s\n", init_flag ? "OK" : "FAIL");
    return init_flag;
}

void AppSDModel::SDDeinit() {
    SD.end();
    init_flag = false;
}

bool AppSDModel::GetInitFlag() {
    return init_flag;
}

void AppSDModel::ClearInitFlag() {
    init_flag = false;
}

bool AppSDModel::IsSDCardExist() {
    return (bool)!((M5.In_I2C.readRegister8(AW9523_ADDR, 0x00, 100000L) >> 4) &
                   0x01);
}
