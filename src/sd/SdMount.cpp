#include "sd/SdMount.h"

#include <SD.h>
#include <SPI.h>
#include "M5Unified.h"
#include "config.h"

static int s_ref = 0;

bool SdCardPresent() {
    return (bool)!((M5.In_I2C.readRegister8(AW9523_ADDR, 0x00, 100000L) >> 4) &
                   0x01);
}

bool SdIsMounted() {
    return s_ref > 0;
}

bool SdMount() {
    if (s_ref > 0) {
        s_ref++;
        return true;
    }

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

    SPI.end();
    SPI.begin(sclk, miso, mosi, cs);

    if (!SD.begin((uint8_t)cs, SPI, 25000000)) {
        USBSerial.println("[SD] mount FAIL");
        return false;
    }
    s_ref = 1;
    USBSerial.printf("[SD] mount OK (sclk=%d miso=%d mosi=%d cs=%d)\n", sclk,
                     miso, mosi, cs);
    return true;
}

void SdUnmount() {
    if (s_ref <= 0) {
        return;
    }
    s_ref--;
    if (s_ref == 0) {
        SD.end();
        USBSerial.println("[SD] unmount");
    }
}
