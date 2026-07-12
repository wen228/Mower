/**
 * EzData2 fleet snapshot: soc / used_mAh / running.
 * Triggers: motor start + every EZ2_UPLOAD_INTERVAL_MS.
 * Protocol: https://docs.m5stack.com/en/guide/ezdata/ezdata_v2_protocol
 */
#pragma once

#define EZ2_WIFI_SSID        ""
#define EZ2_WIFI_PASS        ""
#define EZ2_DEVICE_TOKEN     ""
#define EZ2_MAC_NO_COLON     ""

/* Official MQTT (EzData2 §4.2) */
#define EZ2_MQTT_HOST        "uiflow2.m5stack.com"
#define EZ2_MQTT_PORT        1883
#define EZ2_WIFI_TIMEOUT_MS  15000
#define EZ2_FLUSH_MS         500
#define EZ2_UPLOAD_INTERVAL_MS  (60UL * 1000UL)
