/**
 * EzData2 short-session MQTT: fleet snapshot (soc / used_mAh / running).
 * First upload uses requestType 100 (create fields); later 101 (update).
 * Debug: dump SOC TX JSON, print /down RX, then 104 GET soc.
 */
#ifndef EZDATA2_CLIENT_H
#define EZDATA2_CLIENT_H

#include <Arduino.h>

class EzData2Client {
public:
    void begin();
    void poll();
    bool uploadNow();

private:
    bool ensureWifi_();
    bool publishStatusSnapshot_();
    bool publishField_(const char* name, const char* value, bool dump_json);
    bool publishGet_(const char* name);
    void mqttPump_(uint32_t ms);

    bool began_              = false;
    bool last_running_       = false;
    bool fields_created_     = false; /* after first successful 100s */
    uint32_t last_upload_ms_ = 0;
};

extern EzData2Client g_ez2;

#endif
