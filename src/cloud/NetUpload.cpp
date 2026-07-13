#include "cloud/NetUpload.h"

#include <cstring>

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "cloud/EzData2Client.h"
#include "sd/SdLogger.h"

static QueueHandle_t s_q = nullptr;

static void netWorkerTask(void* /*arg*/) {
    char path[32];
    for (;;) {
        if (xQueueReceive(s_q, path, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        USBSerial.printf("[NET] RTOS upload start %s\n", path);
        const bool ok = g_ez2.uploadLogFile(path);
        g_sd_logger.releaseMountAfterUpload();
        USBSerial.printf("[NET] RTOS upload end ok=%d\n", ok ? 1 : 0);
    }
}

void NetUpload_begin() {
    if (s_q) {
        return;
    }
    s_q = xQueueCreate(1, sizeof(char[32]));
    if (!s_q) {
        USBSerial.println("[NET] RTOS queue fail");
        return;
    }
    /* Low prio: UI / motor on loop keep running. Stack for TLS HTTP. */
    const BaseType_t ok =
        xTaskCreate(netWorkerTask, "netWorker", 12288, nullptr, 1, nullptr);
    if (ok != pdPASS) {
        USBSerial.println("[NET] RTOS task fail");
        return;
    }
    USBSerial.println("[NET] RTOS netWorker ready");
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
