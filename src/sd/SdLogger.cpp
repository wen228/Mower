#include "sd/SdLogger.h"

#include <cstdio>
#include <cstring>

#include "sd/SdMount.h"
#include "motor/Mower.h"

SdLogger g_sd_logger;

bool SdLogger::start() {
    if (file_open_) {
        return true;
    }
    if (!SdCardPresent()) {
        snprintf(status_, sizeof(status_), "Log: no card");
        USBSerial.println("[LOG] start fail: no card");
        return false;
    }
    if (!held_mount_ && !SdMount()) {
        snprintf(status_, sizeof(status_), "Log: mount fail");
        USBSerial.println("[LOG] start fail: mount");
        return false;
    }
    held_mount_ = true;

    if (!openNewFile()) {
        if (held_mount_) {
            SdUnmount();
            held_mount_ = false;
        }
        return false;
    }
    writeRow();
    last_write_ms_ = millis();
    return true;
}

void SdLogger::stop() {
    closeFile();
    if (held_mount_) {
        SdUnmount();
        held_mount_ = false;
    }
    if (path_[0] != '\0' && lines_ > 0) {
        snprintf(status_, sizeof(status_), "saved %s n=%lu", path_,
                 (unsigned long)lines_);
    } else {
        snprintf(status_, sizeof(status_), "Log: STOP");
    }
    USBSerial.println("[LOG] stop");
}

void SdLogger::toggle() {
    if (file_open_) {
        stop();
    } else {
        start();
    }
}

uint16_t SdLogger::nextFileIndex() {
    uint16_t max_n = 0;
    File root = SD.open("/");
    if (!root) {
        return 1;
    }
    File f = root.openNextFile();
    while (f) {
        const char* name = f.name();
        const char* base = strrchr(name, '/');
        base = base ? base + 1 : name;
        unsigned n = 0;
        if (sscanf(base, "mower_%u.csv", &n) == 1 && n > max_n && n < 1000) {
            max_n = (uint16_t)n;
        }
        f = root.openNextFile();
    }
    root.close();
    return (uint16_t)(max_n + 1);
}

bool SdLogger::openNewFile() {
    if (file_open_) {
        return true;
    }

    file_idx_ = nextFileIndex();
    if (file_idx_ == 0 || file_idx_ > 999) {
        file_idx_ = 1;
    }

    snprintf(path_, sizeof(path_), "/mower_%03u.csv", (unsigned)file_idx_);
    file_ = SD.open(path_, FILE_WRITE);
    if (!file_) {
        snprintf(status_, sizeof(status_), "Log: open fail");
        USBSerial.printf("[LOG] open %s FAIL\n", path_);
        return false;
    }

    file_.println("ms,rpm,current_mA,power_W,gear,running,fault");
    file_.flush();
    file_open_ = true;
    lines_     = 0;
    snprintf(status_, sizeof(status_), "REC %s n=0", path_);
    USBSerial.printf("[LOG] REC %s\n", path_);
    return true;
}

void SdLogger::writeRow() {
    if (!file_open_) {
        return;
    }
    const Mower::Status s = g_mower.status();
    file_.printf("%lu,%.1f,%.1f,%.2f,%d,%d,%d\n",
                 (unsigned long)millis(), (double)s.speed, (double)s.current,
                 (double)s.batt_power_w, s.gear, s.running ? 1 : 0,
                 s.fault ? 1 : 0);
    lines_++;
    if ((lines_ % 10) == 0) {
        file_.flush();
    }
    snprintf(status_, sizeof(status_), "REC %s n=%lu", path_,
             (unsigned long)lines_);
}

void SdLogger::closeFile() {
    if (!file_open_) {
        return;
    }
    file_.flush();
    file_.close();
    file_open_ = false;
    USBSerial.printf("[LOG] closed %s lines=%lu\n", path_,
                     (unsigned long)lines_);
}

void SdLogger::poll() {
    if (!file_open_) {
        return;
    }
    const uint32_t now = millis();
    if (now - last_write_ms_ >= kPeriodMs) {
        last_write_ms_ = now;
        writeRow();
    }
}
