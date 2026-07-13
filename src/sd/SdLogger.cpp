#include "sd/SdLogger.h"

#include <cstdio>
#include <cstring>
#include <ctime>

#include <M5Unified.h>

#include "sd/SdMount.h"
#include "motor/Mower.h"
#include "config_mower.h"
#include "cloud/NetUpload.h"

SdLogger g_sd_logger;

void SdLogger::ensureTimeBase_() {
    if (time_base_ok_) {
        return;
    }
    /* One-shot RTC read (no NTP). Then advance with millis only. */
    wall_ms0_ = 0;
    if (M5.Rtc.isEnabled()) {
        m5::rtc_datetime_t dt;
        if (M5.Rtc.getDateTime(&dt)) {
            tm tm_ = dt.get_tm();
            const time_t sec = mktime(&tm_);
            if (sec > (time_t)0) {
                wall_ms0_ = (uint64_t)sec * 1000ULL;
            }
        }
    }
    millis0_      = millis();
    time_base_ok_ = true;
    USBSerial.printf("[LOG] t base wall_ms=%llu millis0=%u (RTC once)\n",
                     (unsigned long long)wall_ms0_, (unsigned)millis0_);
}

uint64_t SdLogger::nowT_ms_() const {
    return wall_ms0_ + (uint32_t)(millis() - millis0_);
}

bool SdLogger::start() {
    if (file_open_) {
        return true;
    }
    ensureTimeBase_();
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
    /* Keep SD mounted; netWorker task uploads then releaseMountAfterUpload. */
    if (path_[0] != '\0' && lines_ > 0) {
        NetUpload_request(path_);
        snprintf(status_, sizeof(status_), "saved %s n=%lu", path_,
                 (unsigned long)lines_);
    } else {
        if (held_mount_) {
            SdUnmount();
            held_mount_ = false;
        }
        snprintf(status_, sizeof(status_), "Log: STOP");
    }
    USBSerial.println("[LOG] stop");
}

void SdLogger::releaseMountAfterUpload() {
    if (held_mount_ && !file_open_) {
        SdUnmount();
        held_mount_ = false;
    }
}

void SdLogger::toggle() {
    if (file_open_) {
        stop();
    } else {
        start();
    }
}

bool SdLogger::openNewFile() {
    if (file_open_) {
        return true;
    }

    /* Name: Mower_MMDD_HHMMSS.csv from RTC at REC start (fallback 0000_000000). */
    int mon = 0, day = 0, hh = 0, mi = 0, se = 0;
    if (M5.Rtc.isEnabled()) {
        m5::rtc_datetime_t dt;
        if (M5.Rtc.getDateTime(&dt)) {
            mon = dt.date.month;
            day = dt.date.date;
            hh  = dt.time.hours;
            mi  = dt.time.minutes;
            se  = dt.time.seconds;
        }
    }
    snprintf(path_, sizeof(path_), "/Mower_%02d%02d_%02d%02d%02d.csv", mon,
             day, hh, mi, se);
    /* FILE_WRITE truncates; remove first so old size never confuses list tools. */
    SD.remove(path_);
    file_ = SD.open(path_, FILE_WRITE);
    if (!file_) {
        snprintf(status_, sizeof(status_), "Log: open fail");
        USBSerial.printf("[LOG] open %s FAIL\n", path_);
        return false;
    }

    file_.println(
        "t,tgt,rpm,current_mA,power_W,gear,running,fault,load,temp");
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
    const float tgt = (float)s.target_raw / SCALE_SPEED_DIV;
    /* t: epoch-ms from RTC baseline + millis delta (single column). */
    file_.printf("%llu,%.1f,%.1f,%.1f,%.2f,%d,%d,%d,%d,%d\n",
                 (unsigned long long)nowT_ms_(), (double)tgt, (double)s.speed,
                 (double)s.current, (double)s.batt_power_w, s.gear,
                 s.running ? 1 : 0, s.fault ? 1 : 0, (int)s.load, s.temp);
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
