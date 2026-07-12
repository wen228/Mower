#include <stdarg.h>
#include <stdio.h>
#include "AppSDView.h"

using namespace Page;

void AppSDView::Create(lv_obj_t* root) {
    ui.img_bg = lv_img_create(root);
    lv_img_set_src(ui.img_bg, ResourcePool::GetImage("app_sd"));

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

    ui.btn_top_center = lv_btn_create(root);
    lv_obj_remove_style_all(ui.btn_top_center);
    lv_obj_set_size(ui.btn_top_center, 170, 68);
    lv_obj_align(ui.btn_top_center, LV_ALIGN_TOP_MID, 0, 0);

    /* File list — leave bottom strip for log toggle */
    ui.cont_file = lv_obj_create(root);
    lv_obj_remove_style_all(ui.cont_file);
    lv_obj_set_size(ui.cont_file, 320, 132);
    lv_obj_clear_flag(ui.cont_file, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(ui.cont_file, 0, 68);

    ui.file_list = lv_list_create(ui.cont_file);
    lv_obj_set_style_radius(ui.file_list, 0, 0);
    lv_obj_set_size(ui.file_list, 300, 120);
    lv_obj_set_pos(ui.file_list, 10, 6);

    ui.label_notice = lv_label_create(root);
    lv_label_set_text(ui.label_notice, "Please insert SD card...");
    lv_obj_align(ui.label_notice, LV_ALIGN_CENTER, 0, 10);
    lv_label_set_long_mode(ui.label_notice, LV_LABEL_LONG_SCROLL);
    lv_obj_set_size(ui.label_notice, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    ui.btn_log = lv_btn_create(root);
    lv_obj_set_size(ui.btn_log, 100, 32);
    lv_obj_set_pos(ui.btn_log, 12, 204);
    lv_obj_set_style_bg_color(ui.btn_log, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(ui.btn_log, 6, 0);
    ui.label_log_btn = lv_label_create(ui.btn_log);
    lv_label_set_text(ui.label_log_btn, "REC");
    lv_obj_center(ui.label_log_btn);

    ui.label_log = lv_label_create(root);
    lv_label_set_text(ui.label_log, "Log: STOP");
    lv_obj_set_style_text_color(ui.label_log, lv_color_hex(0x333333), 0);
    lv_obj_set_pos(ui.label_log, 120, 210);
    lv_label_set_long_mode(ui.label_log, LV_LABEL_LONG_DOT);
    lv_obj_set_width(ui.label_log, 188);
}

void AppSDView::Delete() {
}
