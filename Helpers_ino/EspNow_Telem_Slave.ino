/*
 * EspNow_Telem_Slave — AtomS3R + Atomic Display Base
 * ESP-NOW RX + big-text dashboard (M5Unified / M5AtomDisplay).
 *
 * Board: AtomS3R | Libraries: M5Unified, M5GFX
 * Docs: https://docs.m5stack.com/en/arduino/projects/atomic/atomic_display_base
 *
 * Channel MUST match CoreS3 [ESP] log "ch=N" (WiFi AP → often 11; offline fallback 6).
 * Payload ver=2 includes tgt RPM (closed-loop setpoint).
 */

#include <M5AtomDisplay.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>

/* Match CoreS3 serial: [ESP] ... ch=?? */
#define ESPNOW_CHANNEL 11

/* UI + Serial refresh (ms). RX is event-driven; display is 1 Hz. */
#define UI_PERIOD_MS 1000
#define LINK_LOST_MS 2500

#define ESPNOW_TELEM_MAGIC 0x4D57
#define ESPNOW_TELEM_VER   2
#define ESPNOW_F_READY    (1u << 0)
#define ESPNOW_F_RUNNING  (1u << 1)
#define ESPNOW_F_FAULT    (1u << 2)
#define ESPNOW_F_BATT_LOW (1u << 3)

struct __attribute__((packed)) EspNowTelem {
  uint16_t magic;
  uint8_t  ver;
  uint8_t  flags;
  uint32_t seq;
  int16_t  speed;      /* measured RPM */
  int16_t  tgt;        /* target RPM */
  int16_t  current_mA;
  int16_t  vin_x100;
  int16_t  power_x100;
  uint16_t soc_x10;
  uint8_t  gear;
  uint8_t  load;
};

static volatile uint32_t s_rx = 0;
static volatile uint32_t s_last_rx_ms = 0;
static EspNowTelem s_last;
static bool s_have = false;

static const char* gearName(uint8_t g) {
  switch (g) {
    case 1: return "ECO";
    case 2: return "NORMAL";
    case 3: return "TURBO";
    default: return "?";
  }
}

static const char* loadName(uint8_t L) {
  switch (L) {
    case 0: return "Off";
    case 1: return "Light";
    case 2: return "Medium";
    case 3: return "Heavy";
    case 4: return "Stall";
    default: return "?";
  }
}

/* Arduino-ESP32 3.x / IDF 5 recv cb */
void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  if (len < (int)sizeof(EspNowTelem)) {
    return;
  }
  EspNowTelem t;
  memcpy(&t, data, sizeof(t));
  if (t.magic != ESPNOW_TELEM_MAGIC || t.ver != ESPNOW_TELEM_VER) {
    return;
  }
  s_last = t;
  s_have = true;
  s_rx++;
  s_last_rx_ms = millis();
}

static void drawWaiting(const char* msg) {
  M5.Display.fillScreen(TFT_BLACK);
  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(20, 40);
  M5.Display.println("MOWER");
  M5.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
  M5.Display.setCursor(20, 120);
  M5.Display.println(msg);
}

static void drawDashboard(const EspNowTelem& t, uint32_t age_ms) {
  const bool fault   = (t.flags & ESPNOW_F_FAULT) != 0;
  const bool running = (t.flags & ESPNOW_F_RUNNING) != 0;
  const bool lost    = age_ms > LINK_LOST_MS;

  M5.Display.fillScreen(TFT_BLACK);

  int y = 24;
  const int step = M5.Display.fontHeight() + 8;

  M5.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5.Display.setCursor(20, y);
  M5.Display.print("MOWER");
  y += step;

  if (lost) {
    M5.Display.setTextColor(TFT_ORANGE, TFT_BLACK);
    M5.Display.setCursor(20, y);
    M5.Display.print("LINK LOST");
  } else if (fault) {
    M5.Display.setTextColor(TFT_RED, TFT_BLACK);
    M5.Display.setCursor(20, y);
    M5.Display.printf("FAULT | %s", gearName(t.gear));
  } else if (running) {
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.setCursor(20, y);
    M5.Display.printf("RUN | %s", gearName(t.gear));
  } else {
    M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
    M5.Display.setCursor(20, y);
    M5.Display.printf("STOP | %s", gearName(t.gear));
  }
  y += step;

  M5.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Display.setCursor(20, y);
  M5.Display.printf("RPM  %d    TGT  %d", (int)t.speed, (int)t.tgt);
  y += step;

  M5.Display.setCursor(20, y);
  /* current field is mA (same as AppMower I:). */
  M5.Display.printf("I %.1fmA  Vin %.2f  P %.2fW",
                    (double)t.current_mA, t.vin_x100 / 100.0,
                    t.power_x100 / 100.0);
  y += step;

  M5.Display.setCursor(20, y);
  M5.Display.printf("Load %s    SOC %.1f%%", loadName(t.load),
                    t.soc_x10 / 10.0);
  y += step;

  M5.Display.setTextColor(TFT_DARKGREY, TFT_BLACK);
  M5.Display.setCursor(20, y);
  M5.Display.printf("link seq %lu  age %.1fs  rx %lu",
                    (unsigned long)t.seq, age_ms / 1000.0,
                    (unsigned long)s_rx);
}

void setup() {
  auto cfg = M5.config();
  cfg.external_display.atom_display = true;
  /* Change if your monitor needs another mode. */
  cfg.atom_display.logical_width  = 1920;
  cfg.atom_display.logical_height = 1080;
  M5.begin(cfg);

  M5.setPrimaryDisplayType({m5::board_t::board_M5AtomDisplay});

  {
    int ts = M5.Display.height() / 14;
    if (ts < 4) {
      ts = 4;
    }
    if (ts > 12) {
      ts = 12;
    }
    M5.Display.setTextSize(ts);
  }
  drawWaiting("waiting ESP-NOW...");

  Serial.begin(115200);
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP] slave init fail");
    drawWaiting("ESP-NOW init FAIL");
    return;
  }
  esp_now_register_recv_cb(onRecv);

  Serial.println("EspNow_Telem_Slave");
  Serial.print("MAC ");
  Serial.println(WiFi.macAddress());
  Serial.printf("Listen ch=%u  UI=%ums\n", (unsigned)WiFi.channel(),
                (unsigned)UI_PERIOD_MS);
}

void loop() {
  static uint32_t last_ui = 0;
  const uint32_t now = millis();
  if (last_ui != 0 && (now - last_ui) < UI_PERIOD_MS) {
    delay(20);
    return;
  }
  last_ui = now;

  if (!s_have) {
    drawWaiting("waiting ESP-NOW...");
    Serial.println("[ESP] waiting...");
    return;
  }

  EspNowTelem t = s_last;
  const uint32_t age = now - s_last_rx_ms;
  drawDashboard(t, age);

  Serial.printf(
      "[ESP] seq=%lu run=%d fault=%d gear=%u rpm=%d tgt=%d I=%d Vin=%.2f "
      "P=%.2f SOC=%.1f load=%u age=%.1f rx=%lu\n",
      (unsigned long)t.seq, (t.flags & ESPNOW_F_RUNNING) ? 1 : 0,
      (t.flags & ESPNOW_F_FAULT) ? 1 : 0, (unsigned)t.gear, (int)t.speed,
      (int)t.tgt, (int)t.current_mA, t.vin_x100 / 100.0, t.power_x100 / 100.0,
      t.soc_x10 / 10.0, (unsigned)t.load, age / 1000.0, (unsigned long)s_rx);
}
