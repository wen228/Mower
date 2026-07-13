/**
 * Minimal ESP-NOW master: broadcast g_mower.status() at ESPNOW_TX_PERIOD_MS.
 */
#ifndef ESP_NOW_TX_H
#define ESP_NOW_TX_H

void EspNowTx_begin();
void EspNowTx_poll();

#endif
