#include "cloud/NetUpload.h"

#include <cstdio>
#include <cstring>

#include <Arduino.h>
#include <M5Unified.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "cloud/EzData2Client.h"
#include "sd/SdLogger.h"

static QueueHandle_t s_q = nullptr;

void Ez_hhmmss(char* buf, size_t n) {
    if (!buf || n < 9) {
        return;
    }
    buf[0] = '\0';
    if (M5.Rtc.isEnabled()) {
        m5::rtc_datetime_t dt;
        if (M5.Rtc.getDateTime(&dt)) {
            snprintf(buf, n, "%02d:%02d:%02d", dt.time.hours, dt.time.minutes,
                     dt.time.seconds);
        }
    }
    if (buf[0] == '\0') {
        snprintf(buf, n, "--:--:--");
    }
}

static void netWorkerTask(void* /*arg*/) {
    char path[32];
    char ts[12];
    for (;;) {
        if (xQueueReceive(s_q, path, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        Ez_hhmmss(ts, sizeof(ts));
        USBSerial.printf("[EZ] %s RTOS upload start %s\n", ts, path);
        (void)g_ez2.uploadLogFile(path); /* logs [EZ] HH:MM:SS HTTP / ok */
        g_sd_logger.releaseMountAfterUpload();
    }
}

void NetUpload_begin() {
    if (s_q) {
        return;
    }
    s_q = xQueueCreate(1, sizeof(char[32]));
    if (!s_q) {
        USBSerial.println("[EZ] RTOS queue fail");
        return;
    }
    /* Low prio: UI / motor on loop keep running. Stack for TLS HTTP. */
    const BaseType_t ok =
        xTaskCreate(netWorkerTask, "netWorker", 12288, nullptr, 1, nullptr);
    if (ok != pdPASS) {
        USBSerial.println("[EZ] RTOS task fail");
        return;
    }
    USBSerial.println("[EZ] RTOS netWorker ready");
}

void NetUpload_request(const char* path) {
    if (!s_q || !path || path[0] == '\0') {
        return;
    }
    char buf[32];
    strncpy(buf, path, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    /* Depth 1: overwrite if a previous request still waiting. */
    xQueueOverwrite(s_q, buf);
}
