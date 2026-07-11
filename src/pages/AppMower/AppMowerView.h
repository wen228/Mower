#ifndef __APPMOWER_VIEW_H
#define __APPMOWER_VIEW_H

#include "../Page.h"

namespace Page {

class AppMowerView {
   public:
    void Create(lv_obj_t* root);
    void Delete();

   public:
    struct {
        lv_obj_t* img_bg;
        lv_obj_t* imgbtn_home;
        lv_obj_t* imgbtn_next;
        lv_obj_t* label_title; /* unused while reusing Power banner art */
        lv_obj_t* label_status;
        lv_obj_t* label_telem;
        lv_obj_t* btn_eco;
        lv_obj_t* btn_normal;
        lv_obj_t* btn_turbo;
        lv_obj_t* btn_toggle;
        lv_obj_t* btn_estop;
        lv_obj_t* btn_reset;
        lv_obj_t* label_toggle;
    } ui;

   private:
    lv_obj_t* makeBtn(lv_obj_t* parent, const char* text, lv_coord_t w,
                      lv_coord_t h, lv_color_t bg);
};

}  // namespace Page

#endif
