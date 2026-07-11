#include "AppBattery.h"
#include "config_battery.h"

using namespace Page;

AppBattery::AppBattery() : timer(nullptr) {
}

AppBattery::~AppBattery() {
}

void AppBattery::onCustomAttrConfig() {
    LV_LOG_USER(__func__);
}

void AppBattery::onViewLoad() {
    LV_LOG_USER(__func__);
    View.Create(_root);
    Model.Init();

    AttachEvent(View.ui.imgbtn_home, LV_EVENT_CLICKED);
    AttachEvent(View.ui.imgbtn_next, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_full, LV_EVENT_CLICKED);
}

void AppBattery::onViewDidLoad() {
    LV_LOG_USER(__func__);
}

void AppBattery::onViewWillAppear() {
    LV_LOG_USER(__func__);
    timer = lv_timer_create(onTimerUpdate, 300, this);
    Update();
}

void AppBattery::onViewDidAppear() {
    LV_LOG_USER(__func__);
}

void AppBattery::onViewWillDisappear() {
    LV_LOG_USER(__func__);
}

void AppBattery::onViewDidDisappear() {
    LV_LOG_USER(__func__);
    lv_timer_del(timer);
}

void AppBattery::onViewUnload() {
    LV_LOG_USER(__func__);
    View.Delete();
}

void AppBattery::onViewDidUnload() {
    LV_LOG_USER(__func__);
}

void AppBattery::AttachEvent(lv_obj_t* obj, lv_event_code_t code) {
    lv_obj_set_user_data(obj, this);
    lv_obj_add_event_cb(obj, onEvent, code, this);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void AppBattery::Update() {
    const Mower::Status s = Model.status();

    char buf[48];
    snprintf(buf, sizeof(buf), "SOC: %.1f%%", s.batt_soc_pct);
    lv_label_set_text(View.ui.label_soc, buf);

    int v = (int)(s.batt_soc_pct + 0.5f);
    if (v < 0) {
        v = 0;
    }
    if (v > 100) {
        v = 100;
    }
    lv_bar_set_value(View.ui.bar_soc, v, LV_ANIM_OFF);

    lv_color_t bar_col = lv_color_hex(0x22CC66);
    if (s.batt_low || s.batt_soc_pct <= BATTERY_LOW_SOC_PCT) {
        bar_col = lv_color_hex(0xEE3333);
        lv_obj_set_style_text_color(View.ui.label_soc, lv_color_hex(0xFF5555),
                                    0);
    } else if (s.batt_soc_pct < 40.0f) {
        bar_col = lv_color_hex(0xE6C200);
        lv_obj_set_style_text_color(View.ui.label_soc, lv_color_hex(0x00E5FF),
                                    0);
    } else {
        lv_obj_set_style_text_color(View.ui.label_soc, lv_color_hex(0x00E5FF),
                                    0);
    }
    lv_obj_set_style_bg_color(View.ui.bar_soc, bar_col, LV_PART_INDICATOR);

    char telem[128];
    snprintf(telem, sizeof(telem),
             "P:%.2fW   V:%.2f   I:%.1f\n"
             "used:%.1fmAh   E:%.1fmWh\n"
             "%s  %s %d%%  load:%s",
             s.batt_power_w, s.vin, s.current, s.batt_used_mah, s.batt_energy_mwh,
             s.ready ? (s.running ? "RUN" : "STOP") : "NO ROLLER",
             Mower::gearName(s.gear), Mower::gearPct(s.gear),
             Mower::loadName(s.load));
    lv_label_set_text(View.ui.label_telem, telem);

    if (!s.ready) {
        lv_label_set_text(View.ui.label_warn, "Roller not found");
    } else if (s.batt_low) {
        lv_label_set_text(View.ui.label_warn, "BATT LOW — Full / clear fault");
    } else if (s.fault) {
        lv_label_set_text(View.ui.label_warn, "FAULT latched");
    } else {
        lv_label_set_text(View.ui.label_warn, "");
    }
}

void AppBattery::onTimerUpdate(lv_timer_t* timer) {
    AppBattery* instance = (AppBattery*)timer->user_data;
    instance->Update();
}

void AppBattery::onEvent(lv_event_t* event) {
    AppBattery* instance = (AppBattery*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj        = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code != LV_EVENT_CLICKED) {
        return;
    }

    if (obj == instance->View.ui.imgbtn_home) {
        M5.Speaker.playWav((const uint8_t*)ResourcePool::GetWav("select_0_5s"),
                           ~0u, 1, 1);
        instance->_Manager->Replace("Pages/HomeMenu");
        return;
    }
    if (obj == instance->View.ui.imgbtn_next) {
        M5.Speaker.playWav((const uint8_t*)ResourcePool::GetWav("select_0_5s"),
                           ~0u, 1, 1);
        USBSerial.println("AppBattery -> AppMower");
        instance->_Manager->Replace("Pages/AppMower");
        return;
    }
    if (obj == instance->View.ui.btn_full) {
        instance->Model.resetEnergy();
        instance->Update();
    }
}
