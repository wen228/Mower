#include <stdarg.h>
#include <stdio.h>
#include "HomeMenuView.h"

using namespace Page;

/*
    Menu tile: 60 x 73
    Grid: x = 10 + 80 * (i % 4), y = 75 + 80 * (i / 4)
*/

LV_IMG_DECLARE(menu_power);
#if defined(M5CORES3)
LV_IMG_DECLARE(menu_imu);
#elif defined(M5CORES3SE)
LV_IMG_DECLARE(menu_imu_se);
#endif
LV_IMG_DECLARE(menu_touch);
LV_IMG_DECLARE(menu_i2c);
/* Dummy icon for AppMower / AppBattery until custom assets arrive. */
LV_IMG_DECLARE(menu_wifi);
LV_IMG_DECLARE(menu_sys);

/* Order = AppPower / IMU / Touch / I2C / Mower / Battery */
static const lv_img_dsc_t* MENU_GRID_IMG[] = {
    &menu_power,
#if defined(M5CORES3)
    &menu_imu,
#elif defined(M5CORES3SE)
    &menu_imu_se,
#endif
    &menu_touch,
    &menu_i2c,
    &menu_wifi, /* AppMower  — dummy (WiFi asset) */
    &menu_wifi, /* AppBattery — dummy (WiFi asset) */
};

void HomeMenuView::Create(lv_obj_t* root) {
#if defined(M5CORES3)
    ui.img_bg = lv_img_create(root);
    lv_img_set_src(ui.img_bg, ResourcePool::GetImage("menu_bg"));
#elif defined(M5CORES3SE)
    ui.img_bg_se = lv_img_create(root);
    lv_img_set_src(ui.img_bg_se, ResourcePool::GetImage("menu_bg_se"));
#endif

    for (size_t i = 0; i < 9; i++) {
        ui.imgbtn_list[i] = nullptr;
    }

    /* Main grid: Power / IMU / Touch / I2C / Mower / Battery */
    for (size_t i = 0; i < GRID_COUNT; i++) {
        ui.imgbtn_list[i] = lv_imgbtn_create(root);
        lv_obj_set_size(ui.imgbtn_list[i], 60, 73);
        lv_obj_set_pos(ui.imgbtn_list[i], 10 + 80 * (int)(i % 4),
                       75 + 80 * (int)(i / 4));
        lv_imgbtn_set_src(ui.imgbtn_list[i], LV_IMGBTN_STATE_PRESSED, NULL,
                          MENU_GRID_IMG[i], NULL);
        lv_imgbtn_set_src(ui.imgbtn_list[i], LV_IMGBTN_STATE_RELEASED, NULL,
                          MENU_GRID_IMG[i], NULL);
    }

    /* Sys / RTC — original top-right slot */
    ui.imgbtn_list[SYS_INDEX] = lv_imgbtn_create(root);
    lv_obj_set_size(ui.imgbtn_list[SYS_INDEX], 60, 73);
    lv_obj_set_pos(ui.imgbtn_list[SYS_INDEX], 250, 10);
    lv_imgbtn_set_src(ui.imgbtn_list[SYS_INDEX], LV_IMGBTN_STATE_PRESSED, NULL,
                      &menu_sys, NULL);
    lv_imgbtn_set_src(ui.imgbtn_list[SYS_INDEX], LV_IMGBTN_STATE_RELEASED, NULL,
                      &menu_sys, NULL);
}

void HomeMenuView::Delete() {
}
