#include "cloud/EzData2Client.h"

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <cstdio>
#include <cstring>

#include "config_ezdata2.h"
#include "motor/Mower.h"

EzData2Client g_ez2;

static WiFiClient s_wifi;
static PubSubClient s_mqtt(s_wifi);

static char s_topic_up[96];
static char s_client_id[40];

void EzData2Client::begin() {
    began_          = true;
    last_running_   = false;
    last_upload_ms_ = 0;
}

void EzData2Client::poll() {
    if (!began_) {
        begin();
    }
    if (EZ2_DEVICE_TOKEN[0] == '\0' || EZ2_MAC_NO_COLON[0] == '\0') {
        return;
    }

    const Mower::Status s = g_mower.status();
    const bool running    = s.running;
    const uint32_t now    = millis();

    /* Motor start edge: one snapshot (lower priority than interval, still useful). */
    const bool just_started = running && !last_running_;
    last_running_           = running;

    bool due_interval = false;
    if (last_upload_ms_ == 0) {
        /* First interval after boot only after full period (start edge handles go). */
        last_upload_ms_ = now;
    } else if ((now - last_upload_ms_) >= EZ2_UPLOAD_INTERVAL_MS) {
        due_interval = true;
    }

    if (!just_started && !due_interval) {
        return;
    }

    if (uploadNow()) {
        last_upload_ms_ = millis();
    } else if (due_interval) {
        /* Avoid hammering WiFi on failure every loop */
        last_upload_ms_ = now;
    }
}

bool EzData2Client::uploadNow() {
    if (!began_) {
        begin();
    }
    if (EZ2_DEVICE_TOKEN[0] == '\0' || EZ2_MAC_NO_COLON[0] == '\0') {
        USBSerial.println("[EZ2] skip");
        return false;
    }
    return publishStatusSnapshot_();
}

bool EzData2Client::ensureWifi_() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    if (EZ2_WIFI_SSID[0] == '\0') {
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(EZ2_WIFI_SSID, EZ2_WIFI_PASS);

    const uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > EZ2_WIFI_TIMEOUT_MS) {
            return false;
        }
        delay(200);
    }
    return true;
}

bool EzData2Client::publishField_(const char* name, const char* value) {
    StaticJsonDocument<256> doc;
    doc["deviceToken"]         = EZ2_DEVICE_TOKEN;
    doc["body"]["name"]        = name;
    doc["body"]["value"]       = value;
    doc["body"]["requestType"] = 101;

    char buf[256];
    const size_t n = serializeJson(doc, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
        return false;
    }
    if (!s_mqtt.publish(s_topic_up, buf)) {
        return false;
    }
    delay(30);
    return true;
}

bool EzData2Client::publishStatusSnapshot_() {
    const Mower::Status s = g_mower.status();

    USBSerial.printf("[EZ2] run=%d soc=%.1f used=%.1f\n", s.running ? 1 : 0,
                     (double)s.batt_soc_pct, (double)s.batt_used_mah);

    if (!ensureWifi_()) {
        USBSerial.println("[EZ2] fail wifi");
        return false;
    }

    snprintf(s_client_id, sizeof(s_client_id), "ez%sez", EZ2_MAC_NO_COLON);
    snprintf(s_topic_up, sizeof(s_topic_up), "$ezdata/%s/up", EZ2_DEVICE_TOKEN);

    s_mqtt.setServer(EZ2_MQTT_HOST, EZ2_MQTT_PORT);
    s_mqtt.setBufferSize(512);

    if (!s_mqtt.connect(s_client_id, EZ2_DEVICE_TOKEN, "")) {
        USBSerial.printf("[EZ2] fail mqtt rc=%d\n", s_mqtt.state());
        return false;
    }

    char v[32];
    bool ok = true;

    snprintf(v, sizeof(v), "%d", s.running ? 1 : 0);
    if (!publishField_("running", v)) {
        ok = false;
    }
    dtostrf((double)s.batt_soc_pct, 0, 1, v);
    if (!publishField_("soc", v)) {
        ok = false;
    }
    dtostrf((double)s.batt_used_mah, 0, 1, v);
    if (!publishField_("used_mAh", v)) {
        ok = false;
    }

    const uint32_t t0 = millis();
    while (millis() - t0 < EZ2_FLUSH_MS) {
        s_mqtt.loop();
        delay(10);
    }

    s_mqtt.disconnect();
    USBSerial.printf("[EZ2] %s\n", ok ? "ok" : "fail pub");
    return ok;
}
