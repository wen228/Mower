#include "AppI2C.h"

#include <WiFi.h>
#include <cstdio>

#include "config.h"
#include "config_mower.h"
#include "motor/Mower.h"
#include "sd/SdMount.h"

using namespace Page;

static const lv_color_t kColOk  = lv_color_hex(0x1A7A4C); /* green */
static const lv_color_t kColNg  = lv_color_hex(0xC9A227); /* yellow */
static const lv_color_t kColDis = lv_color_hex(0xFF4444); /* red — Cam only */

AppI2C::AppI2C() {
}

AppI2C::~AppI2C() {
}

void AppI2C::onCustomAttrConfig() {
    LV_LOG_USER(__func__);
}

void AppI2C::onViewLoad() {
    LV_LOG_USER(__func__);
    View.Create(_root);

    AttachEvent(View.ui.imgbtn_home, LV_EVENT_CLICKED);
    AttachEvent(View.ui.imgbtn_next, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_recheck, LV_EVENT_CLICKED);
}

void AppI2C::onViewDidLoad() {
    LV_LOG_USER(__func__);
}

void AppI2C::onViewWillAppear() {
    LV_LOG_USER(__func__);
    runCheckAndShow();
}

void AppI2C::onViewDidAppear() {
    LV_LOG_USER(__func__);
}

void AppI2C::onViewWillDisappear() {
    LV_LOG_USER(__func__);
}

void AppI2C::onViewDidDisappear() {
    LV_LOG_USER(__func__);
}

void AppI2C::onViewUnload() {
    LV_LOG_USER(__func__);
    View.Delete();
}

void AppI2C::onViewDidUnload() {
    LV_LOG_USER(__func__);
}

void AppI2C::AttachEvent(lv_obj_t* obj, lv_event_code_t code) {
    lv_obj_set_user_data(obj, this);
    lv_obj_add_event_cb(obj, onEvent, code, this);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

static void addRow(lv_obj_t* parent, const char* name, bool ok) {
    char buf[40];
    snprintf(buf, sizeof(buf), "%-5s: %s", name, ok ? "OK" : "NG");
    lv_obj_t* lab = lv_label_create(parent);
    lv_label_set_text(lab, buf);
    lv_obj_set_style_text_color(lab, ok ? kColOk : kColNg, 0);
}

static void addCamDisabled(lv_obj_t* parent) {
    lv_obj_t* lab = lv_label_create(parent);
    lv_label_set_text(lab, "Cam  : Disabled");
    lv_obj_set_style_text_color(lab, kColDis, 0);
}

/** Light IMU presence: BMI270 chip-id on internal I2C @ 0x69. */
static bool checkImu() {
    uint8_t id = 0;
    /* Reg 0x00 = chip id; BMI270 → 0x24 */
    if (!M5.In_I2C.readRegister(BMI270_ADDR, 0x00, &id, 1, 100000L)) {
        return false;
    }
    return id == 0x24;
}

/**
 * Live probe Roller on Port A (0x64). Do NOT use g_mower.ready —
 * that flag is latched at App_Init and stays true after unplug.
 */
static bool checkBldcLive() {
    Wire.beginTransmission((uint8_t)ROLLER_I2C_ADDR);
    return Wire.endTransmission() == 0;
}

void AppI2C::runCheckAndShow() {
    lv_obj_clean(View.ui.cont_list);

    const bool bldc = checkBldcLive();
    const bool wifi = (WiFi.status() == WL_CONNECTED);
    const bool sd   = SdCardPresent();
    const bool imu  = checkImu();

    addRow(View.ui.cont_list, "BLDC", bldc);
    addRow(View.ui.cont_list, "WiFi", wifi);
    addRow(View.ui.cont_list, "SD", sd);
    addRow(View.ui.cont_list, "IMU", imu);
    addCamDisabled(View.ui.cont_list);
}

void AppI2C::onEvent(lv_event_t* event) {
    AppI2C* instance = (AppI2C*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj        = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);
    if (code != LV_EVENT_CLICKED) {
        return;
    }

    M5.Speaker.playWav((const uint8_t*)ResourcePool::GetWav("select_0_5s"), ~0u,
                       1, 1);

    if (obj == instance->View.ui.imgbtn_home) {
        instance->_Manager->Replace("Pages/HomeMenu");
        return;
    }
    if (obj == instance->View.ui.imgbtn_next) {
        USBSerial.println("AppI2C(SelfCheck) -> AppPower");
        instance->_Manager->Replace("Pages/AppPower");
        return;
    }
    if (obj == instance->View.ui.btn_recheck) {
        instance->runCheckAndShow();
    }
}
