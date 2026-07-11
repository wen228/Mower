#ifndef __HOMEMENU_VIEW_H
#define __HOMEMENU_VIEW_H

#include "../Page.h"

namespace Page {

class HomeMenuView {
   public:
    void Create(lv_obj_t* root);
    void Delete();

    /** Grid slots 0..5 + Sys/RTC at index 8 (top-right). */
    static const size_t GRID_COUNT = 6;
    static const size_t SYS_INDEX  = 8;

   public:
    struct {
#if defined(M5CORES3)
        lv_obj_t* img_bg;
#elif defined(M5CORES3SE)
        lv_obj_t* img_bg_se;
#endif
        lv_obj_t* imgbtn_list[9];
    } ui;
};

}  // namespace Page

#endif
