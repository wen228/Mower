/*
 * CoreS3_SD_Wipe.ino  — erase ALL files on microSD (FAT delete, not format)
 *
 * Pins (same as M5 docs / your Mower_ui SdMount):
 *   SCK=36  MISO=35  MOSI=37  CS=4
 *
 * Board: Arduino IDE → M5CoreS3 (or M5Stack → CoreS3)
 * Libs:  M5Unified + M5GFX  (or M5CoreS3 package)
 *
 * If mount fails:
 *   1) Full power-cycle (unplug USB), not only Reset button
 *   2) Card contacts face same side as the screen; seat firmly
 *   3) FAT32, preferably ≤16GB
 *   4) Watch DETECT= line (0 = card present on AW9523 P0.4)
 */

#include <M5Unified.h>
#include <SPI.h>
#include <SD.h>
#include <stdarg.h>

#define AW9523_ADDR 0x58

#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN   4

static uint32_t g_files = 0;
static uint32_t g_dirs  = 0;
static uint32_t g_fail  = 0;

void logf(const char *fmt, ...) {
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Serial.print(buf);
    M5.Display.print(buf);
}

void logln(const char *s) {
    Serial.println(s);
    M5.Display.println(s);
}

/* AW9523 P0.4: 0 = TF inserted (active low). */
bool cardDetectPresent() {
    const uint8_t p0 = M5.In_I2C.readRegister8(AW9523_ADDR, 0x00, 100000L);
    return !((p0 >> 4) & 0x01);
}

bool tryMount(uint32_t hz) {
    SD.end();
    SPI.end();
    delay(20);

    pinMode(SD_SPI_CS_PIN, OUTPUT);
    digitalWrite(SD_SPI_CS_PIN, HIGH);

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    delay(10);

    logf("try SD.begin @ %u Hz ... ", (unsigned)hz);
    if (SD.begin((uint8_t)SD_SPI_CS_PIN, SPI, hz)) {
        logln("OK");
        return true;
    }
    logln("FAIL");
    return false;
}

void wipeDir(fs::FS &fs, const char *dirname) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        logf("open fail: %s\n", dirname);
        g_fail++;
        return;
    }

    File entry = root.openNextFile();
    while (entry) {
        String path = entry.path();
        if (path.length() == 0) {
            path = String(dirname);
            if (!path.endsWith("/")) {
                path += "/";
            }
            path += entry.name();
        }
        const bool is_dir = entry.isDirectory();
        entry.close();

        if (is_dir) {
            wipeDir(fs, path.c_str());
            if (fs.rmdir(path.c_str())) {
                logf("rmdir %s\n", path.c_str());
                g_dirs++;
            } else {
                logf("rmdir FAIL %s\n", path.c_str());
                g_fail++;
            }
        } else {
            if (fs.remove(path.c_str())) {
                logf("rm %s\n", path.c_str());
                g_files++;
            } else {
                logf("rm FAIL %s\n", path.c_str());
                g_fail++;
            }
        }
        entry = root.openNextFile();
    }
    root.close();
}

void listRoot() {
    File root = SD.open("/");
    if (!root) {
        logln("list: open / fail");
        return;
    }
    File f = root.openNextFile();
    if (!f) {
        logln("(empty)");
    }
    while (f) {
        if (f.isDirectory()) {
            logf("  DIR  %s\n", f.name());
        } else {
            logf("  FILE %s (%u)\n", f.name(), (unsigned)f.size());
        }
        f = root.openNextFile();
    }
    root.close();
}

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    Serial.begin(115200);
    delay(300);

    M5.Display.setBrightness(60);
    M5.Display.setTextSize(1);
    M5.Display.setTextScroll(true);
    M5.Display.fillScreen(TFT_BLACK);
    M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Display.setCursor(0, 0);

    logln("=== CoreS3 SD WIPE ===");
    logf("board=%d\n", (int)M5.getBoard());

    /* Same power bits UserDemo / Mower use */
    M5.In_I2C.bitOn(AW9523_ADDR, 0x03, 0b10000000, 100000L); /* BOOST_EN */
    delay(50);

    const bool det = cardDetectPresent();
    logf("DETECT(AW9523 P0.4)=%s\n", det ? "PRESENT" : "EMPTY?");
    logf("pins SCK=%d MISO=%d MOSI=%d CS=%d\n", SD_SPI_SCK_PIN,
         SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    /* Multi-speed: 25M often fails after soft reset / shared SPI */
    const uint32_t speeds[] = {25000000, 10000000, 4000000, 1000000, 400000};
    bool ok = false;
    for (uint32_t hz : speeds) {
        if (tryMount(hz)) {
            ok = true;
            break;
        }
        delay(100);
    }

    /* Fallback: leave SPI as M5GFX left it (UserDemo style) */
    if (!ok) {
        SD.end();
        SPI.end();
        delay(20);
        logln("try SD.begin(CS) no re-SPI ...");
        if (SD.begin((uint8_t)SD_SPI_CS_PIN, SPI, 4000000)) {
            logln("OK (fallback)");
            ok = true;
        } else {
            logln("FAIL (fallback)");
        }
    }

    if (!ok) {
        logln("--- MOUNT FAILED ---");
        logln("Check:");
        logln("1 full power unplug USB");
        logln("2 card seated / contacts");
        logln("3 FAT32 <=16GB");
        logln("4 DETECT must be PRESENT");
        while (1) {
            delay(1000);
        }
    }

    logf("Size %lluMB  Used %lluMB\n", SD.cardSize() / (1024 * 1024),
         SD.usedBytes() / (1024 * 1024));

    logln("--- before ---");
    listRoot();

    logln("--- wiping / ---");
    wipeDir(SD, "/");

    logln("--- after ---");
    listRoot();

    logf("done files=%u dirs=%u fail=%u\n", (unsigned)g_files,
         (unsigned)g_dirs, (unsigned)g_fail);
    logln("Safe to reflash app.");
}

void loop() {
    delay(1000);
}
