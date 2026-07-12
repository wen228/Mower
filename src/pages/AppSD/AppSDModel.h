#ifndef __APPSD_MODEL_H
#define __APPSD_MODEL_H

#include <SD.h>
#include "lvgl.h"
#include "M5Unified.h"
#include "config.h"

// TF Detect AW9523 P0.4

namespace Page {

/** Max lines shown in CSV viewer (last N). */
static const int CSV_VIEW_MAX = 50;
/** Display line buffer length. */
static const int CSV_LINE_LEN = 48;

class AppSDModel {
   public:
    bool SDInit();
    void SDDeinit();
    bool GetInitFlag();
    void ClearInitFlag();
    bool IsSDCardExist();

    /**
     * Read CSV (mower log format), keep last CSV_VIEW_MAX data rows.
     * Each out_lines[i] is compact UI: "ms tgt rpm I run fault".
     * @return true if file opened (n may be 0 if empty).
     */
    bool ReadCsvTail50(const char* path, char out_lines[][CSV_LINE_LEN],
                       int* out_n);

   private:
    bool init_flag = false;
};

}  // namespace Page

#endif
