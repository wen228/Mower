/**
 * Shared ESP-NOW payload (CoreS3 TX / AtomS3R RX).
 * Keep packed + fixed types so both sides match.
 */
#pragma once

#include <stdint.h>

#define ESPNOW_TELEM_MAGIC 0x4D57 /* 'MW' */
#define ESPNOW_TELEM_VER   2

/* flags bits */
#define ESPNOW_F_READY    (1u << 0)
#define ESPNOW_F_RUNNING  (1u << 1)
#define ESPNOW_F_FAULT    (1u << 2)
#define ESPNOW_F_BATT_LOW (1u << 3)

struct __attribute__((packed)) EspNowTelem {
    uint16_t magic;      /* ESPNOW_TELEM_MAGIC */
    uint8_t  ver;        /* ESPNOW_TELEM_VER */
    uint8_t  flags;
    uint32_t seq;
    int16_t  speed;      /* measured RPM */
    int16_t  tgt;        /* target RPM (target_raw/100) */
    int16_t  current_mA;
    int16_t  vin_x100;   /* V * 100 */
    int16_t  power_x100; /* W * 100 */
    uint16_t soc_x10;    /* % * 10 */
    uint8_t  gear;       /* 1 Eco / 2 Normal / 3 Turbo */
    uint8_t  load;       /* Mower::Load as uint8 */
};
