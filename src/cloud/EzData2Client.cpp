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
static char s_topic_down[96];
static char s_client_id[40];

/** Last down payload for coarse success check (optional). */
static char s_last_rx[384];
static bool s_last_rx_ok = false;

static void ez2MqttCallback(char* topic, byte* payload, unsigned int length) {
    USBSerial.print("[EZ2] RX json=");
    for (unsigned int i = 0; i < length; i++) {
        USBSerial.print((char)payload[i]);
    }
    USBSerial.println();

    const unsigned int n =
        (length < sizeof(s_last_rx) - 1) ? length : (sizeof(s_last_rx) - 1);
    memcpy(s_last_rx, payload, n);
    s_last_rx[n] = '\0';
    s_last_rx_ok = true;
}

void EzData2Client::begin() {
    began_          = true;
    last_running_   = false;
    last_upload_ms_ = 0;
    /* fields_created_ stays false until first 100 cycle succeeds */
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

    const bool just_started = running && !last_running_;
    last_running_           = running;

    bool due_interval = false;
    if (last_upload_ms_ == 0) {
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

void EzData2Client::mqttPump_(uint32_t ms) {
    const uint32_t t0 = millis();
    while (millis() - t0 < ms) {
        s_mqtt.loop();
        delay(10);
    }
}

bool EzData2Client::publishField_(const char* name, const char* value,
                                  bool dump_json) {
    /*
     * 100 = add field (must exist before 101 update)
     * 101 = modify existing field
     * Doc: does not exist → cmd 500 (what you saw with 101 only)
     */
    const int req = fields_created_ ? 101 : 100;

    StaticJsonDocument<256> doc;
    doc["deviceToken"]         = EZ2_DEVICE_TOKEN;
    doc["body"]["name"]        = name;
    doc["body"]["value"]       = value;
    doc["body"]["requestType"] = req;

    char buf[256];
    const size_t n = serializeJson(doc, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
        return false;
    }
    if (dump_json) {
        USBSerial.printf("[EZ2] TX json(soc %d)=%s\n", req, buf);
    } else {
        USBSerial.printf("[EZ2] TX %s req=%d val=%s\n", name, req, value);
    }
    if (!s_mqtt.publish(s_topic_up, buf)) {
        return false;
    }
    delay(40);
    s_mqtt.loop();
    return true;
}

bool EzData2Client::publishGet_(const char* name) {
    StaticJsonDocument<192> doc;
    doc["deviceToken"]         = EZ2_DEVICE_TOKEN;
    doc["body"]["name"]        = name;
    doc["body"]["requestType"] = 104;

    char buf[192];
    const size_t n = serializeJson(doc, buf, sizeof(buf));
    if (n == 0 || n >= sizeof(buf)) {
        return false;
    }
    USBSerial.printf("[EZ2] TX json(soc 104 GET)=%s\n", buf);
    if (!s_mqtt.publish(s_topic_up, buf)) {
        USBSerial.println("[EZ2] fail pub GET");
        return false;
    }
    delay(40);
    return true;
}

bool EzData2Client::publishStatusSnapshot_() {
    const Mower::Status s = g_mower.status();

    USBSerial.printf("[EZ2] run=%d soc=%.1f used=%.1f create=%d\n",
                     s.running ? 1 : 0, (double)s.batt_soc_pct,
                     (double)s.batt_used_mah, fields_created_ ? 0 : 1);

    if (!ensureWifi_()) {
        USBSerial.println("[EZ2] fail wifi");
        return false;
    }

    snprintf(s_client_id, sizeof(s_client_id), "ez%sez", EZ2_MAC_NO_COLON);
    snprintf(s_topic_up, sizeof(s_topic_up), "$ezdata/%s/up", EZ2_DEVICE_TOKEN);
    snprintf(s_topic_down, sizeof(s_topic_down), "$ezdata/%s/down",
             EZ2_DEVICE_TOKEN);

    s_mqtt.setServer(EZ2_MQTT_HOST, EZ2_MQTT_PORT);
    s_mqtt.setBufferSize(1024);
    s_mqtt.setCallback(ez2MqttCallback);

    if (!s_mqtt.connect(s_client_id, EZ2_DEVICE_TOKEN, "")) {
        USBSerial.printf("[EZ2] fail mqtt rc=%d\n", s_mqtt.state());
        return false;
    }

    if (!s_mqtt.subscribe(s_topic_down)) {
        USBSerial.println("[EZ2] warn: subscribe down failed");
    } else {
        USBSerial.printf("[EZ2] sub %s\n", s_topic_down);
    }
    mqttPump_(200);

    char v[32];
    bool ok = true;
    s_last_rx_ok = false;

    snprintf(v, sizeof(v), "%d", s.running ? 1 : 0);
    if (!publishField_("running", v, false)) {
        ok = false;
    }

    dtostrf((double)s.batt_soc_pct, 0, 1, v);
    if (!publishField_("soc", v, true)) {
        ok = false;
    }

    dtostrf((double)s.batt_used_mah, 0, 1, v);
    if (!publishField_("used_mAh", v, false)) {
        ok = false;
    }

    USBSerial.println("[EZ2] wait RX after pub...");
    mqttPump_(2000);

    /* If create (100) got code 200, mark ready for next time 101 */
    if (!fields_created_ && s_last_rx_ok && strstr(s_last_rx, "\"code\":200")) {
        fields_created_ = true;
        USBSerial.println("[EZ2] fields created → next upload uses 101");
    }
    /* If already existed, server may return 500 on 100; user can create on web.
     * Also: if 101 got 200, fields exist. */
    if (s_last_rx_ok && strstr(s_last_rx, "\"code\":200") &&
        (strstr(s_last_rx, "\"cmd\":100") || strstr(s_last_rx, "\"cmd\":101"))) {
        fields_created_ = true;
    }

    if (!publishGet_("soc")) {
        ok = false;
    }
    USBSerial.println("[EZ2] wait RX after GET...");
    mqttPump_(2000);

    s_mqtt.disconnect();
    USBSerial.printf("[EZ2] %s\n", ok ? "ok" : "fail pub");
    return ok;
}
