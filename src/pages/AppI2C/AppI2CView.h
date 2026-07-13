#ifndef __APPI2C_VIEW_H
#define __APPI2C_VIEW_H

#include "../Page.h"

namespace Page {

class AppI2CView {
public:
    void Create(lv_obj_t* root);
    void Delete();

    struct {
        lv_obj_t* imgbtn_home;
        lv_obj_t* imgbtn_next;
        lv_obj_t* label_title;
        lv_obj_t* cont_list;   /* checklist rows */
        lv_obj_t* btn_recheck;
        lv_obj_t* label_recheck;
    } ui;
};

}  // namespace Page

#endif
