#include "cloud/EzData2Client.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <SD.h>
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
/** Sticky capture for file-upload notify (cmd 105), not overwritten by later RX. */
static char s_file105_rx[384];
static bool s_file105_ok = false;
/** Last multipart form "name" (for 104 GET after upload). */
static char s_last_upload_name[48];

static void ez2MqttCallback(char* topic, byte* payload, unsigned int length) {
    (void)topic;
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

    if (strstr(s_last_rx, "\"cmd\":105") || strstr(s_last_rx, "\"cmd\": 105")) {
        memcpy(s_file105_rx, s_last_rx, n + 1);
        s_file105_ok = true;
    }
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
        USBSerial.println("[EZ2] skip: empty token/mac (need config_ezdata2_secrets.h)");
        return false;
    }
    return publishStatusSnapshot_();
}

bool EzData2Client::ensureWifi_() {
    if (WiFi.status() == WL_CONNECTED) {
        USBSerial.printf("[EZ2] wifi already ip=%s rssi=%d\n",
                         WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    }
    if (EZ2_WIFI_SSID[0] == '\0') {
        USBSerial.println("[EZ2] wifi skip: empty SSID (secrets missing?)");
        return false;
    }

    USBSerial.printf("[EZ2] wifi connecting ssid=%s ...\n", EZ2_WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(EZ2_WIFI_SSID, EZ2_WIFI_PASS);

    const uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - t0 > EZ2_WIFI_TIMEOUT_MS) {
            USBSerial.printf("[EZ2] wifi fail timeout %ums status=%d\n",
                             (unsigned)EZ2_WIFI_TIMEOUT_MS, (int)WiFi.status());
            return false;
        }
        delay(200);
    }
    USBSerial.printf("[EZ2] wifi ok ip=%s rssi=%d ms=%u\n",
                     WiFi.localIP().toString().c_str(), WiFi.RSSI(),
                     (unsigned)(millis() - t0));
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

    /* Short flush only — no multi-second wait, no 104 GET (demo latency). */
    mqttPump_(EZ2_FLUSH_MS);

    /* Prefer RX code:200 if it arrived in the flush window. */
    if (s_last_rx_ok && strstr(s_last_rx, "\"code\":200") &&
        (strstr(s_last_rx, "\"cmd\":100") || strstr(s_last_rx, "\"cmd\":101"))) {
        if (!fields_created_) {
            USBSerial.println("[EZ2] fields ok → next upload uses 101");
        }
        fields_created_ = true;
    } else if (ok && !fields_created_) {
        /* Pubs succeeded; assume create landed (fields already on web in demo). */
        fields_created_ = true;
        USBSerial.println("[EZ2] pubs ok → next upload uses 101");
    }

    s_mqtt.disconnect();
    USBSerial.printf("[EZ2] %s\n", ok ? "ok" : "fail pub");
    return ok;
}

bool EzData2Client::mqttConnectSession_() {
    if (EZ2_DEVICE_TOKEN[0] == '\0' || EZ2_MAC_NO_COLON[0] == '\0') {
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
    mqttPump_(150);
    return true;
}

bool EzData2Client::mqttPublishRaw_(const char* json) {
    if (!json || !s_mqtt.connected()) {
        return false;
    }
    USBSerial.printf("[EZ2] TX %s\n", json);
    if (!s_mqtt.publish(s_topic_up, json)) {
        return false;
    }
    delay(40);
    s_mqtt.loop();
    return true;
}

void EzData2Client::httpGetUrlProbe_(const char* url) {
    if (!url || strncmp(url, "https://", 8) != 0) {
        USBSerial.println("[EZ2] file GET skip: no https url");
        return;
    }
    /* Parse host + path from https://host/path... */
    const char* host_start = url + 8;
    const char* path_start = strchr(host_start, '/');
    if (!path_start) {
        USBSerial.println("[EZ2] file GET fail: bad url");
        return;
    }
    char host[80];
    const size_t host_len = (size_t)(path_start - host_start);
    if (host_len == 0 || host_len >= sizeof(host)) {
        return;
    }
    memcpy(host, host_start, host_len);
    host[host_len] = '\0';

    USBSerial.printf("[EZ2] file GET probe host=%s\n", host);

    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(EZ2_HTTP_TIMEOUT_MS);
    if (!client.connect(host, 443)) {
        USBSerial.println("[EZ2] file GET fail connect");
        return;
    }
    client.printf("GET %s HTTP/1.1\r\n", path_start);
    client.printf("Host: %s\r\n", host);
    client.print("Connection: close\r\n\r\n");

    const uint32_t t0 = millis();
    int status = -1;
    int content_len = -1;
    bool in_body = false;
    char body_snip[128];
    size_t bn = 0;

    while (client.connected() || client.available()) {
        if (millis() - t0 > EZ2_HTTP_TIMEOUT_MS) {
            break;
        }
        if (!client.available()) {
            delay(5);
            continue;
        }
        if (!in_body) {
            String line = client.readStringUntil('\n');
            line.trim();
            if (line.startsWith("HTTP/")) {
                const int sp = line.indexOf(' ');
                if (sp > 0) {
                    status = line.substring(sp + 1).toInt();
                }
            } else if (line.startsWith("Content-Length:") ||
                       line.startsWith("content-length:")) {
                content_len = line.substring(line.indexOf(':') + 1).toInt();
            } else if (line.length() == 0) {
                in_body = true;
            }
            continue;
        }
        while (client.available() && bn + 1 < sizeof(body_snip)) {
            const int c = client.read();
            if (c < 0) {
                break;
            }
            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                body_snip[bn++] = '|';
            } else {
                body_snip[bn++] = (char)c;
            }
        }
        if (bn + 1 >= sizeof(body_snip)) {
            break;
        }
    }
    body_snip[bn] = '\0';
    client.stop();

    USBSerial.printf("[EZ2] file GET status=%d clen=%d snip=%s\n", status,
                     content_len, body_snip);
}

void EzData2Client::verifyUploadedFile_() {
    /*
     * No HTTP GET-file API in EzData2 docs.
     * Verify: (1) MQTT cmd105 URL (2) 104 GET deviceFile (3) HTTPS GET URL.
     */
    USBSerial.println("[EZ2] file verify: wait cmd105 + GET deviceFile");

    if (!s_mqtt.connected() && !mqttConnectSession_()) {
        USBSerial.println("[EZ2] file verify fail mqtt");
        return;
    }

    mqttPump_(EZ2_FILE_VERIFY_MS);

    char url_buf[200];
    url_buf[0] = '\0';
    if (s_file105_ok) {
        USBSerial.printf("[EZ2] file verify got cmd105: %s\n", s_file105_rx);
        const char* v = strstr(s_file105_rx, "\"value\":\"");
        if (v) {
            v += 9;
            const char* e = strchr(v, '"');
            if (e && (size_t)(e - v) < sizeof(url_buf)) {
                memcpy(url_buf, v, (size_t)(e - v));
                url_buf[e - v] = '\0';
                USBSerial.printf("[EZ2] file url=%s\n", url_buf);
            }
        }
    } else {
        USBSerial.println("[EZ2] file verify: no cmd105 in window (try 104)");
    }

    /* 104 GET: prefer form name we uploaded, else name from cmd105, else deviceFile. */
    char get_name_buf[48];
    if (s_last_upload_name[0] != '\0') {
        snprintf(get_name_buf, sizeof(get_name_buf), "%s", s_last_upload_name);
    } else {
        snprintf(get_name_buf, sizeof(get_name_buf), "deviceFile");
    }
    if (s_file105_ok) {
        const char* n = strstr(s_file105_rx, "\"name\":\"");
        if (n) {
            n += 8;
            const char* e = strchr(n, '"');
            if (e && (size_t)(e - n) < sizeof(get_name_buf)) {
                memcpy(get_name_buf, n, (size_t)(e - n));
                get_name_buf[e - n] = '\0';
            }
        }
    }
    USBSerial.printf("[EZ2] file verify 104 name=%s\n", get_name_buf);

    StaticJsonDocument<192> doc;
    doc["deviceToken"]         = EZ2_DEVICE_TOKEN;
    doc["body"]["name"]        = get_name_buf;
    doc["body"]["requestType"] = 104;
    char req[160];
    if (serializeJson(doc, req, sizeof(req)) > 0) {
        s_last_rx_ok = false;
        s_last_rx[0] = '\0';
        if (mqttPublishRaw_(req)) {
            USBSerial.println("[EZ2] file verify: wait 104 RX...");
            mqttPump_(EZ2_FILE_VERIFY_MS);
            if (s_last_rx_ok) {
                USBSerial.printf("[EZ2] file 104 RX=%s\n", s_last_rx);
                if (url_buf[0] == '\0') {
                    const char* v = strstr(s_last_rx, "\"value\":\"");
                    if (v) {
                        v += 9;
                        const char* e = strchr(v, '"');
                        if (e && (size_t)(e - v) < sizeof(url_buf)) {
                            memcpy(url_buf, v, (size_t)(e - v));
                            url_buf[e - v] = '\0';
                            USBSerial.printf("[EZ2] file url(from 104)=%s\n",
                                             url_buf);
                        }
                    }
                }
            } else {
                USBSerial.println("[EZ2] file verify: no 104 RX");
            }
        }
    }

    if (url_buf[0] != '\0') {
        httpGetUrlProbe_(url_buf);
    } else {
        USBSerial.println(
            "[EZ2] file verify: no URL — cloud may store file but GUI/field "
            "not exposed; check my.m5stack.com group");
    }

    s_mqtt.disconnect();
}

bool EzData2Client::uploadLogFile(const char* path) {
    if (!path || path[0] == '\0') {
        USBSerial.println("[EZ2] file skip: empty path");
        return false;
    }
    if (EZ2_DEVICE_TOKEN[0] == '\0') {
        USBSerial.println("[EZ2] file skip: empty token (need secrets)");
        return false;
    }

    File f = SD.open(path, FILE_READ);
    if (!f) {
        USBSerial.printf("[EZ2] file fail open %s\n", path);
        return false;
    }
    const size_t file_size = f.size();
    const char* base = strrchr(path, '/');
    base = (base && base[1]) ? (base + 1) : path;

    USBSerial.printf("[EZ2] file upload %s bytes=%u\n", path,
                     (unsigned)file_size);

    if (!ensureWifi_()) {
        f.close();
        USBSerial.println("[EZ2] file fail wifi");
        return false;
    }

    /*
     * multipart/form-data (EzData2 §3.2).
     * Doc lists deviceToken + file; server also requires form field "name"
     * (error: 名称不能为空). Use CSV basename as the cloud file name.
     * Business path: POST only. Do not call verifyUploadedFile_() here
     * (MQTT 104 / OSS GET stay as library helpers for other ports).
     */
    static const char kBoundary[] = "----MowerEz2Boundary";

    char token_part[192];
    const int token_n = snprintf(
        token_part, sizeof(token_part),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"deviceToken\"\r\n\r\n"
        "%s\r\n",
        kBoundary, EZ2_DEVICE_TOKEN);
    if (token_n <= 0 || token_n >= (int)sizeof(token_part)) {
        f.close();
        USBSerial.println("[EZ2] file fail token part");
        return false;
    }

    /* Required by API though missing from public param table. */
    char name_part[160];
    const int name_n = snprintf(
        name_part, sizeof(name_part),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"name\"\r\n\r\n"
        "%s\r\n",
        kBoundary, base);
    if (name_n <= 0 || name_n >= (int)sizeof(name_part)) {
        f.close();
        USBSerial.println("[EZ2] file fail name part");
        return false;
    }

    char file_head[192];
    const int head_n = snprintf(
        file_head, sizeof(file_head),
        "--%s\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"
        "Content-Type: text/csv\r\n\r\n",
        kBoundary, base);
    if (head_n <= 0 || head_n >= (int)sizeof(file_head)) {
        f.close();
        USBSerial.println("[EZ2] file fail file head");
        return false;
    }

    char tail[64];
    const int tail_n =
        snprintf(tail, sizeof(tail), "\r\n--%s--\r\n", kBoundary);
    if (tail_n <= 0 || tail_n >= (int)sizeof(tail)) {
        f.close();
        return false;
    }

    USBSerial.printf("[EZ2] file form name=%s filename=%s\n", base, base);

    const size_t content_len = (size_t)token_n + (size_t)name_n +
                               (size_t)head_n + file_size + (size_t)tail_n;

    WiFiClientSecure client;
    client.setInsecure(); /* demo: skip CA verify */
    client.setTimeout(EZ2_HTTP_TIMEOUT_MS);

    USBSerial.printf("[EZ2] file HTTPS %s ...\n", EZ2_FILE_UPLOAD_HOST);
    if (!client.connect(EZ2_FILE_UPLOAD_HOST, 443)) {
        f.close();
        USBSerial.println("[EZ2] file fail connect");
        return false;
    }

    client.printf("POST %s HTTP/1.1\r\n", EZ2_FILE_UPLOAD_PATH);
    client.printf("Host: %s\r\n", EZ2_FILE_UPLOAD_HOST);
    client.printf("Content-Type: multipart/form-data; boundary=%s\r\n",
                  kBoundary);
    client.printf("Content-Length: %u\r\n", (unsigned)content_len);
    client.print("Connection: close\r\n\r\n");

    client.write((const uint8_t*)token_part, (size_t)token_n);
    client.write((const uint8_t*)name_part, (size_t)name_n);
    client.write((const uint8_t*)file_head, (size_t)head_n);

    uint8_t buf[512];
    size_t sent = 0;
    while (f.available()) {
        const int n = f.read(buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        const size_t w = client.write(buf, (size_t)n);
        if (w != (size_t)n) {
            f.close();
            client.stop();
            USBSerial.println("[EZ2] file fail write body");
            return false;
        }
        sent += w;
    }
    f.close();
    client.write((const uint8_t*)tail, (size_t)tail_n);

    if (sent != file_size) {
        USBSerial.printf("[EZ2] file warn sent=%u size=%u\n", (unsigned)sent,
                         (unsigned)file_size);
    }

    /* Parse status + skip headers + JSON body (doc: code 200). */
    const uint32_t t0 = millis();
    int status = -1;
    bool headers_done = false;
    char json_body[256];
    size_t jn = 0;
    json_body[0] = '\0';

    while (client.connected() || client.available()) {
        if (millis() - t0 > EZ2_HTTP_TIMEOUT_MS) {
            USBSerial.println("[EZ2] file fail timeout resp");
            client.stop();
            return false;
        }
        if (!client.available()) {
            delay(10);
            continue;
        }
        if (!headers_done) {
            String line = client.readStringUntil('\n');
            line.trim();
            if (line.startsWith("HTTP/")) {
                const int sp = line.indexOf(' ');
                if (sp > 0) {
                    status = line.substring(sp + 1).toInt();
                }
            } else if (line.length() == 0) {
                headers_done = true;
            }
            continue;
        }
        while (client.available() && jn + 1 < sizeof(json_body)) {
            const int c = client.read();
            if (c < 0) {
                break;
            }
            json_body[jn++] = (char)c;
        }
        if (jn + 1 >= sizeof(json_body)) {
            break;
        }
    }
    json_body[jn] = '\0';
    client.stop();

    USBSerial.printf("[EZ2] file HTTP status=%d body=%s\n", status, json_body);

    /* Transport 200 is not enough — API uses body.code (200 ok, 500 fail). */
    const bool biz_fail =
        (strstr(json_body, "\"code\":500") != nullptr) ||
        (strstr(json_body, "\"code\": 500") != nullptr);
    const bool biz_ok =
        (strstr(json_body, "\"code\":200") != nullptr) ||
        (strstr(json_body, "\"code\": 200") != nullptr);
    const bool http_ok = (status == 200) && biz_ok && !biz_fail;

    if (!http_ok) {
        USBSerial.println("[EZ2] file fail (need body code 200; see body above)");
        return false;
    }

    snprintf(s_last_upload_name, sizeof(s_last_upload_name), "%s", base);
    USBSerial.printf("[EZ2] file HTTP ok bytes=%u name=%s\n",
                     (unsigned)file_size, base);
    /* intentionally not: verifyUploadedFile_(); */
    return true;
}
