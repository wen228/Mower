/**
 * EzData2: MQTT fleet snapshot + HTTP CSV file upload (§3.2).
 * MQTT: start + interval, short flush, no GET.
 * HTTP: STOP → multipart POST only. verifyUploadedFile_/httpGetUrlProbe_ kept
 * for other MCUs / manual use — not called from upload path.
 */
#ifndef EZDATA2_CLIENT_H
#define EZDATA2_CLIENT_H

#include <Arduino.h>

class EzData2Client {
public:
    void begin();
    void poll();
    bool uploadNow();
    /** POST SD CSV while card still mounted. path e.g. /Mower_MMDD_HHMMSS.csv */
    bool uploadLogFile(const char* path);

private:
    bool ensureWifi_();
    bool publishStatusSnapshot_();
    bool publishField_(const char* name, const char* value, bool dump_json);
    bool mqttConnectSession_();
    bool mqttPublishRaw_(const char* json);
    void mqttPump_(uint32_t ms);
    void verifyUploadedFile_();
    void httpGetUrlProbe_(const char* url);

    bool began_              = false;
    bool last_running_       = false;
    bool fields_created_     = false; /* after first successful 100s */
    uint32_t last_upload_ms_ = 0;
};

extern EzData2Client g_ez2;

#endif
