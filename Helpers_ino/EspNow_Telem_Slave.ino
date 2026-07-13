/*
 * EspNow_Telem_Slave — AtomS3R + Atomic Display Base
 * ESP-NOW RX + dashboard (M5Unified / M5AtomDisplay).
 *
 * Board: AtomS3R | Libraries: M5Unified, M5GFX
 * Docs: https://docs.m5stack.com/en/arduino/projects/atomic/atomic_display_base
 *
 * Channel MUST match CoreS3 [ESP] log "ch=N".
 * Payload ver=2 includes tgt RPM.
 */

#include <M5AtomDisplay.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <string.h>
#include <stdio.h>

/* Match CoreS3 serial: [ESP] ... ch=?? */
#define ESPNOW_CHANNEL 11

/* UI + Serial refresh (ms). RX is event-driven. */
#define UI_PERIOD_MS 1000
#define LINK_LOST_MS 2500

/* Fixed size — height/14 on 1080p was ~12, far too large. */
#define UI_TEXT_SIZE 3

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
  int16_t  speed;
  int16_t  tgt;
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

/* Last painted state — skip full repaint when unchanged. */
static uint32_t s_drawn_seq = 0xFFFFFFFFu;
static uint8_t  s_drawn_flags = 0xFF;
static bool     s_drawn_lost = true;
static bool     s_screen_cleared = false;

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

/* One line: fixed cursor, black bg text (no full-screen clear). */
static void lineAt(int x, int y, uint16_t fg, const char* s) {
  M5.Display.setTextColor(fg, TFT_BLACK);
  M5.Display.setCursor(x, y);
  /* Pad so shorter strings erase leftovers from longer previous text. */
  M5.Display.printf("%-48s", s);
}

static void drawWaiting(const char* msg) {
  if (!s_screen_cleared) {
    M5.Display.fillScreen(TFT_BLACK);
    s_screen_cleared = true;
  }
  const int step = M5.Display.fontHeight() + 6;
  lineAt(24, 40, TFT_CYAN, "MOWER");
  lineAt(24, 40 + step, TFT_ORANGE, msg);
  s_drawn_seq = 0xFFFFFFFFu;
  s_drawn_lost = true;
}

static void drawDashboard(const EspNowTelem& t, uint32_t age_ms, bool force) {
  const bool fault   = (t.flags & ESPNOW_F_FAULT) != 0;
  const bool running = (t.flags & ESPNOW_F_RUNNING) != 0;
  const bool lost    = age_ms > LINK_LOST_MS;

  /* Skip heavy redraw when nothing meaningful changed (link age only). */
  if (!force && !lost && !s_drawn_lost && t.seq == s_drawn_seq &&
      t.flags == s_drawn_flags) {
    return;
  }

  if (!s_screen_cleared) {
    M5.Display.fillScreen(TFT_BLACK);
    s_screen_cleared = true;
  }

  const int x = 24;
  const int step = M5.Display.fontHeight() + 6;
  int y = 28;
  char buf[64];

  M5.Display.startWrite();

  lineAt(x, y, TFT_CYAN, "MOWER");
  y += step;

  if (lost) {
    lineAt(x, y, TFT_ORANGE, "LINK LOST");
  } else if (fault) {
    snprintf(buf, sizeof(buf), "FAULT | %s", gearName(t.gear));
    lineAt(x, y, TFT_RED, buf);
  } else if (running) {
    snprintf(buf, sizeof(buf), "RUN | %s", gearName(t.gear));
    lineAt(x, y, TFT_GREEN, buf);
  } else {
    snprintf(buf, sizeof(buf), "STOP | %s", gearName(t.gear));
    lineAt(x, y, TFT_YELLOW, buf);
  }
  y += step;

  snprintf(buf, sizeof(buf), "RPM  %d    TGT  %d", (int)t.speed, (int)t.tgt);
  lineAt(x, y, TFT_WHITE, buf);
  y += step;

  snprintf(buf, sizeof(buf), "I %.1fmA  Vin %.2f  P %.2fW",
           (double)t.current_mA, t.vin_x100 / 100.0, t.power_x100 / 100.0);
  lineAt(x, y, TFT_WHITE, buf);
  y += step;

  snprintf(buf, sizeof(buf), "Load %s    SOC %.1f%%", loadName(t.load),
           t.soc_x10 / 10.0);
  lineAt(x, y, TFT_WHITE, buf);
  y += step;

  snprintf(buf, sizeof(buf), "link seq %lu  age %.1fs  rx %lu",
           (unsigned long)t.seq, age_ms / 1000.0, (unsigned long)s_rx);
  lineAt(x, y, TFT_DARKGREY, buf);

  M5.Display.endWrite();

  s_drawn_seq   = t.seq;
  s_drawn_flags = t.flags;
  s_drawn_lost  = lost;
}

void setup() {
  auto cfg = M5.config();
  cfg.external_display.atom_display = true;
  /*
   * Lower logical res = less HDMI pixel work than 1920x1080 fill.
   * Monitor must accept scaled modes (see Atomic Display Base docs).
   */
  cfg.atom_display.logical_width  = 1280;
  cfg.atom_display.logical_height = 720;
  M5.begin(cfg);

  M5.setPrimaryDisplayType({m5::board_t::board_M5AtomDisplay});
  M5.Display.setTextSize(UI_TEXT_SIZE);
  M5.Display.setTextWrap(false);

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
  Serial.printf("Listen ch=%u  UI=%ums  text=%d  %dx%d\n",
                (unsigned)WiFi.channel(), (unsigned)UI_PERIOD_MS, UI_TEXT_SIZE,
                M5.Display.width(), M5.Display.height());
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
  drawDashboard(t, age, false);

  Serial.printf(
      "[ESP] seq=%lu run=%d fault=%d gear=%u rpm=%d tgt=%d I=%d Vin=%.2f "
      "P=%.2f SOC=%.1f load=%u age=%.1f rx=%lu\n",
      (unsigned long)t.seq, (t.flags & ESPNOW_F_RUNNING) ? 1 : 0,
      (t.flags & ESPNOW_F_FAULT) ? 1 : 0, (unsigned)t.gear, (int)t.speed,
      (int)t.tgt, (int)t.current_mA, t.vin_x100 / 100.0, t.power_x100 / 100.0,
      t.soc_x10 / 10.0, (unsigned)t.load, age / 1000.0, (unsigned long)s_rx);
}
