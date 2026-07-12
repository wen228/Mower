/**
 * EzData2 short-session MQTT: fleet-style snapshot (soc / used_mAh / running).
 * - on motor start (running 0→1)
 * - every EZ2_UPLOAD_INTERVAL_MS (default 60s)
 */
#ifndef EZDATA2_CLIENT_H
#define EZDATA2_CLIENT_H

#include <Arduino.h>

class EzData2Client {
public:
    void begin();
    /** Call from main loop (after Mower_poll). */
    void poll();
    /** Immediate short session; for debug. */
    bool uploadNow();

private:
    bool ensureWifi_();
    bool publishStatusSnapshot_();
    bool publishField_(const char* name, const char* value);

    bool began_         = false;
    bool last_running_  = false;
    uint32_t last_upload_ms_ = 0;
};

extern EzData2Client g_ez2;

#endif
