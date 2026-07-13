#include "AppMower.h"
#include "motor/Mower.h"

using namespace Page;

AppMower::AppMower() : timer(nullptr) {
}

AppMower::~AppMower() {
}

void AppMower::onCustomAttrConfig() {
    LV_LOG_USER(__func__);
}

void AppMower::onViewLoad() {
    LV_LOG_USER(__func__);
    View.Create(_root);
    Model.Init();

    AttachEvent(View.ui.imgbtn_home, LV_EVENT_CLICKED);
    AttachEvent(View.ui.imgbtn_next, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_eco, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_normal, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_turbo, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_toggle, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_estop, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_reset, LV_EVENT_CLICKED);
}

void AppMower::onViewDidLoad() {
    LV_LOG_USER(__func__);
}

void AppMower::onViewWillAppear() {
    LV_LOG_USER(__func__);
    timer = lv_timer_create(onTimerUpdate, 200, this);
    Update();
}

void AppMower::onViewDidAppear() {
    LV_LOG_USER(__func__);
}

void AppMower::onViewWillDisappear() {
    LV_LOG_USER(__func__);
}

void AppMower::onViewDidDisappear() {
    LV_LOG_USER(__func__);
    lv_timer_del(timer);
}

void AppMower::onViewUnload() {
    LV_LOG_USER(__func__);
    View.Delete();
}

void AppMower::onViewDidUnload() {
    LV_LOG_USER(__func__);
}

void AppMower::AttachEvent(lv_obj_t* obj, lv_event_code_t code) {
    lv_obj_set_user_data(obj, this);
    lv_obj_add_event_cb(obj, onEvent, code, this);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

/** Demo fault tips for UI only (serial logs unchanged). */
static void faultTip(const char* reason, const char** title, const char** tip) {
    static const struct {
        const char* key;
        const char* title;
        const char* tip;
    } kMap[] = {
        /* English until CJK font includes tip glyphs (see note to user). */
        {"soft stall", "Soft stall", "Lift blade / clear grass, then Reset"},
        {"HW jam", "HW jam", "Power off, free blade, then Reset"},
        {"overvoltage", "Overvoltage", "Check 12V supply, then Reset"},
        {"sys error", "Sys error", "Replug Roller / reboot, Reset"},
        {"battery low", "Low battery", "Stop work or recharge (sim)"},
        {"E-STOP", "E-STOP", "Press Reset, then RUN"},
    };
    *title = "Fault";
    *tip   = "Press Reset, then RUN";
    if (!reason || !reason[0]) {
        return;
    }
    for (size_t i = 0; i < sizeof(kMap) / sizeof(kMap[0]); i++) {
        if (strstr(reason, kMap[i].key)) {
            *title = kMap[i].title;
            *tip   = kMap[i].tip;
            return;
        }
    }
}

void AppMower::Update() {
    const Mower::Status s = Model.status();
    const int pct = Mower::gearPct(s.gear ? s.gear : GEAR_ECO);

    char status[72];
    if (!s.ready) {
        snprintf(status, sizeof(status), "NO ROLLER | %s %d%%",
                 Mower::gearName(s.gear ? s.gear : GEAR_ECO), pct);
    } else if (s.fault) {
        const char* title = "Fault";
        const char* tip   = "";
        faultTip(s.fault_reason, &title, &tip);
        (void)tip;
        snprintf(status, sizeof(status), "FAULT: %s | %s %d%%", title,
                 Mower::gearName(s.gear), pct);
    } else {
        snprintf(status, sizeof(status), "%s | %s %d%%",
                 s.running ? "RUN" : "STOP", Mower::gearName(s.gear), pct);
    }
    lv_label_set_text(View.ui.label_status, status);
    lv_obj_set_style_text_color(
        View.ui.label_status,
        s.fault ? lv_color_hex(0xFF4444)
                : (!s.ready ? lv_color_hex(0xFF4444)
                            : (s.running ? lv_color_hex(0x00FF88)
                                         : lv_color_hex(0xFFCC00))),
        0);

    char telem[160];
    const bool tip_red = !s.ready || s.fault;
    if (!s.ready) {
        snprintf(telem, sizeof(telem),
                 "Check Port A I2C + motor power\n"
                 "Spd:--  I:--  SOC:%.0f%%",
                 s.batt_soc_pct);
    } else if (s.fault) {
        const char* title = "Fault";
        const char* tip   = "Press Reset, then RUN";
        faultTip(s.fault_reason, &title, &tip);
        snprintf(telem, sizeof(telem),
                 "%s\nSpd:%.0f  I:%.1f  SOC:%.0f%%", tip, (double)s.speed,
                 (double)s.current, (double)s.batt_soc_pct);
    } else {
        snprintf(telem, sizeof(telem),
                 "Spd:%.0f  I:%.1f  Vin:%.2f\nLoad:%s  P:%.2fW  SOC:%.0f%%",
                 (double)s.speed, (double)s.current, (double)s.vin,
                 Mower::loadName(s.load), (double)s.batt_power_w,
                 (double)s.batt_soc_pct);
    }
    lv_label_set_text(View.ui.label_telem, telem);
    lv_obj_set_style_text_color(
        View.ui.label_telem,
        tip_red ? lv_color_hex(0xFF4444) : lv_color_hex(0x333333), 0);

    /* Short labels — RUN/E-STOP/Reset share one row. */
    if (View.ui.label_toggle) {
        lv_label_set_text(View.ui.label_toggle, s.running ? "STOP" : "RUN");
    }
}

void AppMower::onTimerUpdate(lv_timer_t* timer) {
    AppMower* instance = (AppMower*)timer->user_data;
    instance->Update();
}

void AppMower::onEvent(lv_event_t* event) {
    AppMower* instance = (AppMower*)lv_event_get_user_data(event);
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
        USBSerial.println("[Mow] AppMower -> AppBattery");
        instance->_Manager->Replace("Pages/AppBattery");
        return;
    }

    if (obj == instance->View.ui.btn_eco) {
        g_mower.setGear(GEAR_ECO);
    } else if (obj == instance->View.ui.btn_normal) {
        g_mower.setGear(GEAR_NORMAL);
    } else if (obj == instance->View.ui.btn_turbo) {
        g_mower.setGear(GEAR_TURBO);
    } else if (obj == instance->View.ui.btn_toggle) {
        g_mower.toggle();
    } else if (obj == instance->View.ui.btn_estop) {
        g_mower.eStop();
    } else if (obj == instance->View.ui.btn_reset) {
        g_mower.clearFault();
    }

    instance->Update();
}
