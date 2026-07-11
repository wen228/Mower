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

void AppMower::Update() {
    const Mower::Status s = Model.status();
    const int pct = Mower::gearPct(s.gear ? s.gear : GEAR_ECO);

    char status[64];
    if (!s.ready) {
        snprintf(status, sizeof(status), "NO ROLLER | %s %d%%",
                 Mower::gearName(s.gear ? s.gear : GEAR_ECO), pct);
    } else if (s.fault) {
        snprintf(status, sizeof(status), "FAULT | %s %d%% | %s",
                 Mower::gearName(s.gear), pct, s.running ? "RUN" : "STOP");
    } else {
        snprintf(status, sizeof(status), "%s | %s %d%%",
                 s.running ? "RUN" : "STOP", Mower::gearName(s.gear), pct);
    }
    lv_label_set_text(View.ui.label_status, status);
    lv_obj_set_style_text_color(
        View.ui.label_status,
        s.fault ? lv_color_hex(0xFF4444)
                : (s.running ? lv_color_hex(0x00FF88) : lv_color_hex(0xFFCC00)),
        0);

    char telem[96];
    snprintf(telem, sizeof(telem), "Spd:%.1f  I:%.1f  Vin:%.2f  Load:%s",
             s.speed, s.current, s.vin, Mower::loadName(s.load));
    lv_label_set_text(View.ui.label_telem, telem);

    if (View.ui.label_toggle) {
        lv_label_set_text(View.ui.label_toggle,
                          s.running ? "STOP (running)" : "RUN (stopped)");
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
        USBSerial.println("AppMower -> AppBattery");
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
