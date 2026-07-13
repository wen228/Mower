#include "cloud/EspNowTx.h"

#include "config_espnow.h"

#if ESPNOW_ENABLE

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <cstring>

#include "espnow_telem.h"
#include "motor/Mower.h"

static const uint8_t kBcast[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF,
                                                 0xFF, 0xFF, 0xFF};

static bool s_ok      = false;
static uint32_t s_seq = 0;
static uint32_t s_last_ms = 0;

static int16_t clamp_i16(float v) {
    if (v > 32767.0f) {
        return 32767;
    }
    if (v < -32768.0f) {
        return -32768;
    }
    return (int16_t)v;
}

static bool addBcastPeer_() {
    esp_now_peer_info_t peer;
    memset(&peer, 0, sizeof(peer));
    memcpy(peer.peer_addr, kBcast, ESP_NOW_ETH_ALEN);
    /* 0 = current STA/AP channel (see esp_now.h). */
    peer.channel = 0;
    peer.encrypt = false;
    peer.ifidx   = WIFI_IF_STA;

    if (esp_now_is_peer_exist(kBcast)) {
        esp_now_del_peer(kBcast);
    }
    esp_err_t e = esp_now_add_peer(&peer);
    if (e != ESP_OK) {
        USBSerial.printf("[ESP] add peer fail %d\n", (int)e);
        return false;
    }
    return true;
}

void EspNowTx_begin() {
    WiFi.mode(WIFI_STA);
    /* No AP yet → pin fallback channel so a slave on same ch can hear us. */
    if (WiFi.status() != WL_CONNECTED) {
        esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
    }

    if (esp_now_init() != ESP_OK) {
        USBSerial.println("[ESP] init fail");
        s_ok = false;
        return;
    }
    if (!addBcastPeer_()) {
        s_ok = false;
        return;
    }

    s_ok      = true;
    s_seq     = 0;
    s_last_ms = 0;
    USBSerial.printf("[ESP] ready mac=%s ch=%u period=%ums\n",
                     WiFi.macAddress().c_str(), (unsigned)WiFi.channel(),
                     (unsigned)ESPNOW_TX_PERIOD_MS);
}

void EspNowTx_poll() {
    if (!s_ok) {
        return;
    }
    const uint32_t now = millis();
    if (s_last_ms != 0 && (now - s_last_ms) < ESPNOW_TX_PERIOD_MS) {
        return;
    }
    s_last_ms = now;

    const Mower::Status st = g_mower.status();

    EspNowTelem p;
    memset(&p, 0, sizeof(p));
    p.magic = ESPNOW_TELEM_MAGIC;
    p.ver   = ESPNOW_TELEM_VER;
    p.flags = 0;
    if (st.ready) {
        p.flags |= ESPNOW_F_READY;
    }
    if (st.running) {
        p.flags |= ESPNOW_F_RUNNING;
    }
    if (st.fault) {
        p.flags |= ESPNOW_F_FAULT;
    }
    if (st.batt_low) {
        p.flags |= ESPNOW_F_BATT_LOW;
    }
    p.seq        = ++s_seq;
    p.speed      = clamp_i16(st.speed);
    /* target_raw ≈ RPM * 100 (same scale as Roller setSpeed). */
    p.tgt        = clamp_i16((float)st.target_raw / 100.0f);
    p.current_mA = clamp_i16(st.current);
    p.vin_x100   = clamp_i16(st.vin * 100.0f);
    p.power_x100 = clamp_i16(st.batt_power_w * 100.0f);
    {
        float s10 = st.batt_soc_pct * 10.0f;
        if (s10 < 0) {
            s10 = 0;
        }
        if (s10 > 65535.0f) {
            s10 = 65535.0f;
        }
        p.soc_x10 = (uint16_t)s10;
    }
    p.gear = (uint8_t)(st.gear > 0 ? st.gear : 1);
    p.load = (uint8_t)st.load;

    esp_err_t e = esp_now_send(kBcast, (const uint8_t*)&p, sizeof(p));
    if (e != ESP_OK) {
        USBSerial.printf("[ESP] send fail seq=%lu err=%d\n",
                         (unsigned long)p.seq, (int)e);
    } else if ((p.seq % 5u) == 0u) {
        /* ~5 s at 1 Hz */
        USBSerial.printf("[ESP] seq=%lu run=%d rpm=%d tgt=%d soc=%.1f ch=%u\n",
                         (unsigned long)p.seq, (p.flags & ESPNOW_F_RUNNING) ? 1 : 0,
                         (int)p.speed, (int)p.tgt, (double)p.soc_x10 / 10.0,
                         (unsigned)WiFi.channel());
    }
}

#else /* !ESPNOW_ENABLE */

void EspNowTx_begin() {}
void EspNowTx_poll() {}

#endif
