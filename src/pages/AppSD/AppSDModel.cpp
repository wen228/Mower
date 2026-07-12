#include "AppSDModel.h"

#include <cstdio>
#include <cstring>

#include "sd/SdMount.h"

using namespace Page;

bool AppSDModel::SDInit() {
    if (init_flag) {
        return true;
    }
    init_flag = SdMount();
    return init_flag;
}

void AppSDModel::SDDeinit() {
    if (!init_flag) {
        return;
    }
    SdUnmount();
    init_flag = false;
}

bool AppSDModel::GetInitFlag() {
    return init_flag;
}

void AppSDModel::ClearInitFlag() {
    if (init_flag) {
        SdUnmount();
        init_flag = false;
    }
}

bool AppSDModel::IsSDCardExist() {
    return SdCardPresent();
}

bool AppSDModel::ReadCsvTail50(const char* path, char out_lines[][CSV_LINE_LEN],
                               int* out_n) {
    if (!path || !out_lines || !out_n) {
        return false;
    }
    *out_n = 0;

    /* Browse page holds a mount ref; remount if lost. */
    if (!SdIsMounted() && !SdMount()) {
        return false;
    }

    File f = SD.open(path);
    if (!f && path[0] == '/') {
        f = SD.open(path + 1); /* try without leading slash */
    }
    if (!f) {
        return false;
    }

    char ring[CSV_VIEW_MAX][CSV_LINE_LEN];
    int count = 0;
    int w     = 0;
    char raw[96];
    bool first = true;

    while (f.available()) {
        size_t n = f.readBytesUntil('\n', raw, sizeof(raw) - 1);
        raw[n]   = '\0';
        /* strip CR */
        if (n > 0 && raw[n - 1] == '\r') {
            raw[n - 1] = '\0';
        }
        if (raw[0] == '\0') {
            continue;
        }
        if (first) {
            first = false; /* skip header */
            continue;
        }

        unsigned long ms = 0;
        float tgt = 0, rpm = 0, cur = 0;
        int run = 0, fault = 0;
        /* Full CSV: ms,tgt,rpm,current_mA,power_W,gear,running,fault,load,temp
         * Also accept old 7-col files without tgt. */
        int nfield = sscanf(raw, "%lu,%f,%f,%f,%*f,%*d,%d,%d", &ms, &tgt, &rpm,
                            &cur, &run, &fault);
        if (nfield < 6) {
            /* legacy: ms,rpm,I,P,gear,run,fault */
            tgt = 0;
            nfield = sscanf(raw, "%lu,%f,%f,%*f,%*d,%d,%d", &ms, &rpm, &cur,
                            &run, &fault);
            if (nfield < 5) {
                continue;
            }
        }
        /* M5 UI subset, clipped for 320px: ms tgt rpm I run fault */
        snprintf(ring[w], CSV_LINE_LEN, "%lu %.0f %.0f %.0f %d %d", ms,
                 (double)tgt, (double)rpm, (double)cur, run, fault);
        w = (w + 1) % CSV_VIEW_MAX;
        if (count < CSV_VIEW_MAX) {
            count++;
        }
    }
    f.close();

    const int start = (count < CSV_VIEW_MAX) ? 0 : w;
    for (int i = 0; i < count; i++) {
        const int src = (start + i) % CSV_VIEW_MAX;
        memcpy(out_lines[i], ring[src], CSV_LINE_LEN);
    }
    *out_n = count;
    return true;
}
