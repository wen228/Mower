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
     * Read CSV into internal static-sized buffers (not on call stack).
     * UI lines: "MM:SS tgt rpm I run fault", last CSV_VIEW_MAX rows.
     */
    bool ReadCsvTail50(const char* path);
    int viewLineCount() const { return view_n_; }
    const char* viewLine(int i) const;

   private:
    bool init_flag = false;
    int view_n_    = 0;
    char view_lines_[CSV_VIEW_MAX][CSV_LINE_LEN];
};

}  // namespace Page

#endif
