/**
 * Minimal FreeRTOS net worker: CSV upload only (STOP → queue → task).
 */
#ifndef NET_UPLOAD_H
#define NET_UPLOAD_H

#include <stddef.h>

/** Create queue + netWorker task (call once from setup). */
void NetUpload_begin();

/** Queue path for upload; returns immediately. path e.g. /Mower_....csv */
void NetUpload_request(const char* path);

/** Fill buf with HH:MM:SS from RTC (or --:--:--). buf >= 9. */
void Ez_hhmmss(char* buf, size_t n);

#endif
