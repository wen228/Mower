#include "AppMowerView.h"

using namespace Page;

/* Official apps: top chrome ~68px (Power/I2C). Temporary: reuse app_power_ii. */
static const lv_coord_t kBannerH = 68;

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
    lv_obj_set_style_bg_color(root, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

    /* Same pattern as AppPowerView: full Power art; body covers diagram. */
    ui.img_bg = lv_img_create(root);
    lv_img_set_src(ui.img_bg, ResourcePool::GetImage("app_power_ii"));

    lv_obj_t* body = lv_obj_create(root);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, 320, 240 - kBannerH);
    lv_obj_set_pos(body, 0, kBannerH);
    lv_obj_set_style_bg_color(body, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_clear_flag(body, LV_OBJ_FLAG_SCROLLABLE);

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

    /* Title is inside Power banner art; keep slot unused for now. */
    ui.label_title = nullptr;

    /* Status + telemetry (room for 2-line telem). */
    ui.label_status = lv_label_create(root);
    lv_label_set_text(ui.label_status, "STOP | Eco 30%");
    lv_obj_set_style_text_color(ui.label_status, lv_color_hex(0x1A7A4C), 0);
    lv_obj_set_pos(ui.label_status, 12, kBannerH + 6);

    ui.label_telem = lv_label_create(root);
    lv_label_set_text(ui.label_telem, "Spd:--  I:--  Vin:--\nLoad:--  P:--  SOC:--");
    lv_obj_set_style_text_color(ui.label_telem, lv_color_hex(0x333333), 0);
    lv_obj_set_pos(ui.label_telem, 12, kBannerH + 24);

    /* Gear row */
    lv_color_t gear_bg = lv_color_hex(0x3A5FCD);
    ui.btn_eco    = makeBtn(root, "Eco", 94, 34, gear_bg);
    ui.btn_normal = makeBtn(root, "Normal", 94, 34, gear_bg);
    ui.btn_turbo  = makeBtn(root, "Turbo", 94, 34, gear_bg);
    lv_obj_set_pos(ui.btn_eco, 12, kBannerH + 62);
    lv_obj_set_pos(ui.btn_normal, 113, kBannerH + 62);
    lv_obj_set_pos(ui.btn_turbo, 214, kBannerH + 62);

    /* RUN / E-STOP / Reset on one row (short labels). */
    ui.btn_toggle = makeBtn(root, "RUN", 96, 38, lv_color_hex(0x2E8B57));
    ui.btn_estop  = makeBtn(root, "E-STOP", 96, 38, lv_color_hex(0xCC2222));
    ui.btn_reset  = makeBtn(root, "Reset", 96, 38, lv_color_hex(0x666666));
    lv_obj_set_pos(ui.btn_toggle, 12, kBannerH + 108);
    lv_obj_set_pos(ui.btn_estop, 112, kBannerH + 108);
    lv_obj_set_pos(ui.btn_reset, 212, kBannerH + 108);
    ui.label_toggle = lv_obj_get_child(ui.btn_toggle, 0);
}

void AppMowerView::Delete() {
}
