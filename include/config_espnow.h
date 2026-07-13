/**
 * ESP-NOW broadcast (CoreS3 master → remote display).
 * Arduino-ESP32 2.0 classic esp_now.h — not ESP32_NOW.h (3.x).
 */
#pragma once

#ifndef ESPNOW_ENABLE
#define ESPNOW_ENABLE 1
#endif

/* Broadcast period (ms). 1 Hz — easy on WiFi RF. */
#ifndef ESPNOW_TX_PERIOD_MS
#define ESPNOW_TX_PERIOD_MS 1000
#endif

/* Used when STA is not associated (no AP channel yet). Slave must match. */
#ifndef ESPNOW_CHANNEL
#define ESPNOW_CHANNEL 6
#endif
