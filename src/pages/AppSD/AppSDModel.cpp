#include "AppSDModel.h"

#include <cstdio>
#include <cstring>

#include "sd/SdMount.h"

using namespace Page;

/* File-scope ring — avoid ~2.4KB on loopTask stack (overflow on empty/small open). */
static char s_ring[CSV_VIEW_MAX][CSV_LINE_LEN];
static char s_raw[96];

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

const char* AppSDModel::viewLine(int i) const {
    if (i < 0 || i >= view_n_) {
        return "";
    }
    return view_lines_[i];
}

bool AppSDModel::ReadCsvTail50(const char* path) {
    view_n_ = 0;
    if (!path) {
        return false;
    }

    if (!SdIsMounted() && !SdMount()) {
        return false;
    }

    File f = SD.open(path);
    if (!f && path[0] == '/') {
        f = SD.open(path + 1);
    }
    if (!f) {
        return false;
    }

    int count = 0;
    int w     = 0;
    bool first = true;

    while (f.available()) {
        size_t n = f.readBytesUntil('\n', s_raw, sizeof(s_raw) - 1);
        s_raw[n] = '\0';
        if (n > 0 && s_raw[n - 1] == '\r') {
            s_raw[n - 1] = '\0';
        }
        if (s_raw[0] == '\0') {
            continue;
        }
        if (first) {
            first = false; /* skip header (even if file only has header) */
            continue;
        }

        unsigned long ms = 0;
        float tgt = 0, rpm = 0, cur = 0;
        int run = 0, fault = 0;
        int nfield = sscanf(s_raw, "%lu,%f,%f,%f,%*f,%*d,%d,%d", &ms, &tgt,
                            &rpm, &cur, &run, &fault);
        if (nfield < 6) {
            tgt    = 0;
            nfield = sscanf(s_raw, "%lu,%f,%f,%*f,%*d,%d,%d", &ms, &rpm, &cur,
                            &run, &fault);
            if (nfield < 5) {
                continue;
            }
        }
        /* Fixed-width spaces only (default font, no extra typeface). */
        snprintf(s_ring[w], CSV_LINE_LEN, "%6lu %4.0f %4.0f %4.0f %3d %5d", ms,
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
        memcpy(view_lines_[i], s_ring[src], CSV_LINE_LEN);
    }
    view_n_ = count;
    return true;
}
