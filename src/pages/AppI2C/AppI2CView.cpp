#include "AppI2CView.h"

using namespace Page;

void AppI2CView::Create(lv_obj_t* root) {
    lv_obj_set_style_bg_color(root, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    ui.imgbtn_home = lv_imgbtn_create(root);
    lv_obj_set_size(ui.imgbtn_home, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_imgbtn_set_src(ui.imgbtn_home, LV_IMGBTN_STATE_RELEASED, NULL,
                      ResourcePool::GetImage("home_r"), NULL);
    lv_imgbtn_set_src(ui.imgbtn_home, LV_IMGBTN_STATE_PRESSED, NULL,
                      ResourcePool::GetImage("home_p"), NULL);
    lv_obj_align(ui.imgbtn_home, LV_ALIGN_TOP_LEFT, 0, 0);

    ui.imgbtn_next = lv_imgbtn_create(root);
    lv_obj_set_size(ui.imgbtn_next, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_imgbtn_set_src(ui.imgbtn_next, LV_IMGBTN_STATE_RELEASED, NULL,
                      ResourcePool::GetImage("next_r"), NULL);
    lv_imgbtn_set_src(ui.imgbtn_next, LV_IMGBTN_STATE_PRESSED, NULL,
                      ResourcePool::GetImage("next_p"), NULL);
    lv_obj_align(ui.imgbtn_next, LV_ALIGN_TOP_RIGHT, 5, 0);

    ui.label_title = lv_label_create(root);
    lv_label_set_text(ui.label_title, "Device Self-Check");
    lv_obj_set_style_text_color(ui.label_title, lv_color_hex(0x222222), 0);
    lv_obj_align(ui.label_title, LV_ALIGN_TOP_MID, 0, 22);

    ui.cont_list = lv_obj_create(root);
    lv_obj_remove_style_all(ui.cont_list);
    lv_obj_set_size(ui.cont_list, 300, 140);
    lv_obj_set_pos(ui.cont_list, 10, 60);
    lv_obj_set_style_bg_color(ui.cont_list, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(ui.cont_list, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ui.cont_list, 8, 0);
    lv_obj_set_style_pad_all(ui.cont_list, 8, 0);
    lv_obj_set_flex_flow(ui.cont_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui.cont_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(ui.cont_list, 4, 0);
    lv_obj_clear_flag(ui.cont_list, LV_OBJ_FLAG_SCROLLABLE);

    ui.btn_recheck = lv_btn_create(root);
    lv_obj_set_size(ui.btn_recheck, 200, 36);
    lv_obj_align(ui.btn_recheck, LV_ALIGN_BOTTOM_MID, 0, -12);
    lv_obj_set_style_bg_color(ui.btn_recheck, lv_color_hex(0x3A5FCD), 0);
    lv_obj_set_style_radius(ui.btn_recheck, 6, 0);
    ui.label_recheck = lv_label_create(ui.btn_recheck);
    lv_label_set_text(ui.label_recheck, "Recheck");
    lv_obj_center(ui.label_recheck);
}

void AppI2CView::Delete() {
}
