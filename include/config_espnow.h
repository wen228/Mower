/**
 * ESP-NOW broadcast (CoreS3 master → remote display).
 * Arduino-ESP32 2.0 classic esp_now.h — not ESP32_NOW.h (3.x).
 */
#pragma once

#ifndef ESPNOW_ENABLE
#define ESPNOW_ENABLE 1
#endif

/* Broadcast period (ms). 0.5 Hz — less RF contention with WiFi STA. */
#ifndef ESPNOW_TX_PERIOD_MS
#define ESPNOW_TX_PERIOD_MS 2000
#endif

/* Used when STA is not associated (no AP channel yet). Slave must match. */
#ifndef ESPNOW_CHANNEL
#define ESPNOW_CHANNEL 6
#endif
