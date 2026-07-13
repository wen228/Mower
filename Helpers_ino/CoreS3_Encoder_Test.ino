/*
 * CoreS3 / CoreS3 Lite — Unit Encoder (U135) standalone test
 *
 * Official examples only cover M5Core / Core2. This sketch uses M5Unified
 * so it runs on CoreS3 / CoreS3 Lite.
 *
 * Your wiring (DIN Base Port C, I2C over UART pins):
 *   Yellow → G17 = SDA
 *   White  → G18 = SCL
 *   Encoder I2C addr 0x40
 *
 * If raw never moves: set ENC_SWAP_PINS 1 (swap SDA/SCL).
 * If you plug Encoder on Port A instead: set ENC_USE_PORT_A 1
 *   (CoreS3 Port A: SDA=G2, SCL=G1 — same bus as Roller if both plugged).
 *
 * Arduino IDE:
 *   Board: M5CoreS3 (or M5Stack → CoreS3)
 *   Libs:  M5Unified, M5GFX, M5Unit-Encoder
 * Serial: 115200
 */

#include <M5Unified.h>
#include <Wire.h>
#include <Unit_Encoder.h>

// ---- wiring ----
#ifndef ENC_USE_PORT_A
#define ENC_USE_PORT_A  0   /* 0=Port C Wire1, 1=Port A Wire */
#endif
#ifndef ENC_SWAP_PINS
#define ENC_SWAP_PINS   0   /* 1 = swap SDA/SCL if reads look stuck */
#endif

#define ENC_ADDR  0x40

#if ENC_USE_PORT_A
  #define ENC_SDA  2
  #define ENC_SCL  1
  #define ENC_WIRE Wire
#else
  #if ENC_SWAP_PINS
    #define ENC_SDA  18
    #define ENC_SCL  17
  #else
    #define ENC_SDA  17
    #define ENC_SCL  18
  #endif
  #define ENC_WIRE Wire1
#endif

Unit_Encoder enc;
int16_t last_val = 0;
uint32_t last_print = 0;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);
  Serial.begin(115200);
  delay(300);

  M5.Display.setBrightness(60);
  M5.Display.setTextSize(2);
  M5.Display.setTextScroll(true);
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5.Display.setCursor(0, 0);

  Serial.println("=== CoreS3 Encoder Test ===");
  Serial.printf("bus=%s sda=%d scl=%d addr=0x%02X swap=%d\n",
#if ENC_USE_PORT_A
                "Wire/PortA",
#else
                "Wire1/PortC",
#endif
                ENC_SDA, ENC_SCL, ENC_ADDR, ENC_SWAP_PINS);

  M5.Display.printf("Encoder Test\n");
  M5.Display.printf("sda=%d scl=%d\n", ENC_SDA, ENC_SCL);

  ENC_WIRE.end();
  delay(10);
  enc.begin(&ENC_WIRE, ENC_ADDR, (uint8_t)ENC_SDA, (uint8_t)ENC_SCL, 100000);

  ENC_WIRE.beginTransmission(ENC_ADDR);
  uint8_t err = ENC_WIRE.endTransmission();
  if (err != 0) {
    Serial.printf("PROBE FAIL err=%u\n", err);
    M5.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5.Display.printf("PROBE FAIL %u\n", err);
    M5.Display.printf("Check Port C cable\n");
    while (1) {
      delay(1000);
    }
  }

  last_val = enc.getEncoderValue();
  Serial.printf("PROBE OK raw=%d btn=%d\n", (int)last_val,
                (int)enc.getButtonStatus());
  M5.Display.printf("PROBE OK\n");
  enc.setLEDColor(0, 0x001100); /* green both */
}

void loop() {
  int16_t val = enc.getEncoderValue();
  bool btn    = enc.getButtonStatus(); /* example: 0=pressed */

  if (val != last_val) {
    if (val > last_val) {
      enc.setLEDColor(1, 0x000011); /* blue-ish LED1 */
      enc.setLEDColor(2, 0x000000);
    } else {
      enc.setLEDColor(2, 0x110000); /* red LED2 */
      enc.setLEDColor(1, 0x000000);
    }
    Serial.printf("ENC %d -> %d d=%d\n", (int)last_val, (int)val,
                  (int)val - (int)last_val);
    last_val = val;
  }

  if (!btn) {
    enc.setLEDColor(0, 0xC800FF); /* purple when pressed */
  }

  uint32_t now = millis();
  if (now - last_print >= 300) {
    last_print = now;
    M5.Display.fillRect(0, 80, 320, 100, TFT_BLACK);
    M5.Display.setCursor(0, 80);
    M5.Display.printf("val: %d\n", (int)val);
    M5.Display.printf("btn: %s\n", btn ? "UP" : "DOWN");
    Serial.printf("hb val=%d btn=%d\n", (int)val, (int)btn);
  }

  delay(20);
}
