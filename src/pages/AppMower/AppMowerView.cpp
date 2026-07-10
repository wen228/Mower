#include "AppMowerView.h"

using namespace Page;

lv_obj_t* AppMowerView::makeBtn(lv_obj_t* parent, const char* text, lv_coord_t w,
                                lv_coord_t h, lv_color_t bg) {
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_t* lab = lv_label_create(btn);
    lv_label_set_text(lab, text);
    lv_obj_center(lab);
    return btn;
}

void AppMowerView::Create(lv_obj_t* root) {
    lv_obj_set_style_bg_color(root, lv_color_hex(0x202020), 0);
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
    lv_label_set_text(ui.label_title, "AppMower");
    lv_obj_set_style_text_color(ui.label_title, lv_color_white(), 0);
    lv_obj_align(ui.label_title, LV_ALIGN_TOP_MID, 0, 12);

    ui.label_status = lv_label_create(root);
    lv_label_set_text(ui.label_status, "STOP | Eco 30%");
    lv_obj_set_style_text_color(ui.label_status, lv_color_hex(0x00FF88), 0);
    lv_obj_set_pos(ui.label_status, 10, 48);

    ui.label_telem = lv_label_create(root);
    lv_label_set_text(ui.label_telem, "Spd:--  I:--  Vin:--  Load:--");
    lv_obj_set_style_text_color(ui.label_telem, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_pos(ui.label_telem, 10, 70);

    lv_color_t gear_bg = lv_color_hex(0x3A5FCD);
    ui.btn_eco    = makeBtn(root, "Eco", 90, 36, gear_bg);
    ui.btn_normal = makeBtn(root, "Normal", 90, 36, gear_bg);
    ui.btn_turbo  = makeBtn(root, "Turbo", 90, 36, gear_bg);
    lv_obj_set_pos(ui.btn_eco, 12, 100);
    lv_obj_set_pos(ui.btn_normal, 115, 100);
    lv_obj_set_pos(ui.btn_turbo, 218, 100);

    ui.btn_toggle = makeBtn(root, "RUN / STOP", 296, 40, lv_color_hex(0x2E8B57));
    lv_obj_set_pos(ui.btn_toggle, 12, 148);
    ui.label_toggle = lv_obj_get_child(ui.btn_toggle, 0);

    ui.btn_estop = makeBtn(root, "E-STOP", 140, 36, lv_color_hex(0xCC2222));
    ui.btn_reset = makeBtn(root, "Reset", 140, 36, lv_color_hex(0x666666));
    lv_obj_set_pos(ui.btn_estop, 12, 196);
    lv_obj_set_pos(ui.btn_reset, 168, 196);
}

void AppMowerView::Delete() {
}
