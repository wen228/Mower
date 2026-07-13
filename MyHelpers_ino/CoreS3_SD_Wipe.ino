/*
 * CoreS3_SD_Wipe.ino
 * Flash once on M5CoreS3 to delete ALL files/dirs on the microSD card.
 *
 * Based on M5 official SPI pins:
 *   https://github.com/m5stack/M5CoreS3/blob/main/examples/Basic/sdcard/sdcard.ino
 *
 * Hardwares: M5CoreS3 (+ SD in CoreS3 base / DinBase / SD card slot)
 * Libraries: M5CoreS3 (brings M5GFX / M5Unified as deps)
 *
 * WARNING: irreversible delete of everything under "/".
 *          Does not low-level format the card (FAT table may still look used).
 */

#include <M5CoreS3.h>
#include <SPI.h>
#include <SD.h>

#define SD_SPI_SCK_PIN  36
#define SD_SPI_MISO_PIN 35
#define SD_SPI_MOSI_PIN 37
#define SD_SPI_CS_PIN   4

M5Canvas canvas(&CoreS3.Display);

static uint32_t g_files = 0;
static uint32_t g_dirs  = 0;
static uint32_t g_fail  = 0;

void printf_log(const char *format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    Serial.print(buf);
    canvas.print(buf);
    canvas.pushSprite(0, 0);
}

void println_log(const char *str) {
    Serial.println(str);
    canvas.println(str);
    canvas.pushSprite(0, 0);
}

/* Recursively remove everything under dirname (keeps the directory itself). */
void wipeDir(fs::FS &fs, const char *dirname) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        printf_log("open fail: %s\n", dirname);
        g_fail++;
        return;
    }

    File entry = root.openNextFile();
    while (entry) {
        /* Prefer full path; fall back to name if path() empty on some cores. */
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
                printf_log("rmdir %s\n", path.c_str());
                g_dirs++;
            } else {
                printf_log("rmdir FAIL %s\n", path.c_str());
                g_fail++;
            }
        } else {
            if (fs.remove(path.c_str())) {
                printf_log("rm %s\n", path.c_str());
                g_files++;
            } else {
                printf_log("rm FAIL %s\n", path.c_str());
                g_fail++;
            }
        }

        entry = root.openNextFile();
    }
    root.close();
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    File root = fs.open(dirname);
    if (!root || !root.isDirectory()) {
        return;
    }
    File file = root.openNextFile();
    if (!file) {
        println_log("(empty)");
    }
    while (file) {
        if (file.isDirectory()) {
            printf_log("  DIR  %s\n", file.name());
            if (levels) {
                listDir(fs, file.path(), levels - 1);
            }
        } else {
            printf_log("  FILE %s (%u)\n", file.name(), (unsigned)file.size());
        }
        file = root.openNextFile();
    }
    root.close();
}

void setup() {
    CoreS3.begin();
    Serial.begin(115200);

    canvas.setColorDepth(1);
    canvas.createSprite(CoreS3.Display.width(), CoreS3.Display.height());
    canvas.setPaletteColor(1, GREEN);
    canvas.setTextSize((float)canvas.width() / 160);
    canvas.setTextScroll(true);

    println_log("=== CoreS3 SD WIPE ===");
    println_log("ALL files will be deleted!");

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);
    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        println_log("Card failed / not present");
        while (1) {
            delay(1000);
        }
    }

    if (SD.cardType() == CARD_NONE) {
        println_log("No SD card attached");
        while (1) {
            delay(1000);
        }
    }

    printf_log("Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
    printf_log("Used: %lluMB\n", SD.usedBytes() / (1024 * 1024));

    println_log("--- before ---");
    listDir(SD, "/", 2);

    println_log("--- wiping / ---");
    wipeDir(SD, "/");

    println_log("--- after ---");
    listDir(SD, "/", 2);

    printf_log("done files=%u dirs=%u fail=%u\n", (unsigned)g_files,
               (unsigned)g_dirs, (unsigned)g_fail);
    printf_log("Used now: %lluMB\n", SD.usedBytes() / (1024 * 1024));
    println_log("Safe to re-flash your app.");
}

void loop() {
    delay(1000);
}
