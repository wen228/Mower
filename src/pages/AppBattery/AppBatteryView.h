#ifndef __APPBATTERY_VIEW_H
#define __APPBATTERY_VIEW_H

#include "../Page.h"

namespace Page {

class AppBatteryView {
   public:
    void Create(lv_obj_t* root);
    void Delete();

   public:
    struct {
        lv_obj_t* img_bg;
        lv_obj_t* imgbtn_home;
        lv_obj_t* imgbtn_next;
        lv_obj_t* label_title; /* unused while reusing Power banner art */
        lv_obj_t* label_soc;
        lv_obj_t* bar_soc;
        lv_obj_t* label_telem;
        lv_obj_t* label_warn;
        lv_obj_t* btn_full;
    } ui;
};

}  // namespace Page

#endif
