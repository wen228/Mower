/**
 * Manual SD CSV logger (REC / STOP).
 * t = wall ms from one-shot RTC baseline + millis (no NTP, no extra column).
 * t,tgt,rpm,current_mA,power_W,gear,running,fault,load,temp
 */
#ifndef SD_LOGGER_H
#define SD_LOGGER_H

#include <Arduino.h>
#include <SD.h>

class SdLogger {
public:
    /** Start a new session (open file + header). */
    bool start();
    /** Stop session (flush + close). */
    void stop();
    /** Toggle REC <-> STOP. */
    void toggle();

    bool isRecording() const { return file_open_; }
    const char* path() const { return path_; }
    uint32_t lines() const { return lines_; }
    const char* statusText() const { return status_; }

    /** Call every loop after Mower_poll; writes only while recording. */
    void poll();

private:
    bool openNewFile();
    void writeRow();
    void closeFile();
    /** 1..kMaxFiles ring; overwrites oldest slot. */
    uint16_t nextFileIndex();
    /** Once: RTC → wall_ms0_ + millis0_; later t = wall_ms0_ + (millis-millis0_). */
    void ensureTimeBase_();
    uint64_t nowT_ms_() const;

    bool file_open_  = false;
    bool held_mount_ = false;
    bool time_base_ok_ = false;
    uint64_t wall_ms0_ = 0;   /* RTC epoch ms at capture */
    uint32_t millis0_  = 0;   /* millis() at same instant */
    uint32_t last_write_ms_ = 0;
    uint32_t lines_  = 0;
    uint16_t file_idx_ = 0; /* last used 1..kMaxFiles; 0 = none yet */
    File file_;
    char path_[24]   = "";
    char status_[48] = "Log: STOP";

    static const uint32_t kPeriodMs  = 200;
    static const uint16_t kMaxFiles  = 99;
};

extern SdLogger g_sd_logger;

#endif
