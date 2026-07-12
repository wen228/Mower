#include "AppBatteryView.h"
#include "font/lv_font.h"

using namespace Page;

/* Same temporary chrome as AppMower: reuse Power top art. */
static const lv_coord_t kBannerH = 68;

void AppBatteryView::Create(lv_obj_t* root) {
    lv_obj_set_style_bg_color(root, lv_color_hex(0xF5F5F5), 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

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

    ui.label_title = nullptr;

    ui.label_soc = lv_label_create(root);
    lv_label_set_text(ui.label_soc, "SOC: --.-%");
    lv_obj_set_style_text_color(ui.label_soc, lv_color_hex(0x0077AA), 0);
    lv_obj_set_style_text_font(ui.label_soc, &lv_font_montserrat_34, 0);
    lv_obj_align(ui.label_soc, LV_ALIGN_TOP_MID, 0, kBannerH + 10);

    ui.bar_soc = lv_bar_create(root);
    lv_obj_set_size(ui.bar_soc, 280, 28);
    lv_obj_align(ui.bar_soc, LV_ALIGN_TOP_MID, 0, kBannerH + 54);
    lv_bar_set_range(ui.bar_soc, 0, 100);
    lv_bar_set_value(ui.bar_soc, 100, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ui.bar_soc, lv_color_hex(0xDDDDDD), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui.bar_soc, lv_color_hex(0x22CC66),
                              LV_PART_INDICATOR);
    lv_obj_set_style_radius(ui.bar_soc, 6, LV_PART_MAIN);
    lv_obj_set_style_radius(ui.bar_soc, 6, LV_PART_INDICATOR);

    ui.label_telem = lv_label_create(root);
    lv_label_set_text(ui.label_telem,
                      "P:--  V:--  I:--\nused:--  E:--\n--");
    lv_obj_set_style_text_color(ui.label_telem, lv_color_hex(0x333333), 0);
    lv_obj_set_pos(ui.label_telem, 20, kBannerH + 92);

    ui.label_warn = lv_label_create(root);
    lv_label_set_text(ui.label_warn, "");
    lv_obj_set_style_text_color(ui.label_warn, lv_color_hex(0xCC2222), 0);
    lv_obj_set_pos(ui.label_warn, 20, kBannerH + 148);

    ui.btn_full = lv_btn_create(root);
    lv_obj_set_size(ui.btn_full, 110, 30);
    lv_obj_align(ui.btn_full, LV_ALIGN_BOTTOM_RIGHT, -12, -10);
    lv_obj_set_style_bg_color(ui.btn_full, lv_color_hex(0x3A5FCD), 0);
    lv_obj_set_style_radius(ui.btn_full, 6, 0);
    lv_obj_t* lab = lv_label_create(ui.btn_full);
    lv_label_set_text(lab, "Full (b)");
    lv_obj_center(lab);
}

void AppBatteryView::Delete() {
}
