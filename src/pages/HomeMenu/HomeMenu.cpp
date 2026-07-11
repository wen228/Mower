#include "HomeMenu.h"

using namespace Page;

HomeMenu::HomeMenu() : timer(nullptr) {
}

HomeMenu::~HomeMenu() {
}

void HomeMenu::onCustomAttrConfig() {
    LV_LOG_USER(__func__);
}

void HomeMenu::onViewLoad() {
    LV_LOG_USER(__func__);

    View.Create(_root);

    for (size_t i = 0; i < HomeMenuView::GRID_COUNT; i++) {
        AttachEvent(View.ui.imgbtn_list[i], LV_EVENT_CLICKED);
    }
    AttachEvent(View.ui.imgbtn_list[HomeMenuView::SYS_INDEX], LV_EVENT_CLICKED);
}

void HomeMenu::onViewDidLoad() {
    LV_LOG_USER(__func__);
}

void HomeMenu::onViewWillAppear() {
    LV_LOG_USER(__func__);

    timer = lv_timer_create(onTimerUpdate, 1000, this);
}

void HomeMenu::onViewDidAppear() {
    LV_LOG_USER(__func__);
}

void HomeMenu::onViewWillDisappear() {
    LV_LOG_USER(__func__);
}

void HomeMenu::onViewDidDisappear() {
    LV_LOG_USER(__func__);
    lv_timer_del(timer);
}

void HomeMenu::onViewUnload() {
    LV_LOG_USER(__func__);
}

void HomeMenu::onViewDidUnload() {
    LV_LOG_USER(__func__);
}

void HomeMenu::AttachEvent(lv_obj_t* obj, lv_event_code_t code) {
    lv_obj_set_user_data(obj, this);
    lv_obj_add_event_cb(obj, onEvent, code, this);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void HomeMenu::Update() {
}

void HomeMenu::onTimerUpdate(lv_timer_t* timer) {
    HomeMenu* instance = (HomeMenu*)timer->user_data;

    instance->Update();
}

void HomeMenu::onEvent(lv_event_t* event) {
    HomeMenu* instance = (HomeMenu*)lv_event_get_user_data(event);
    LV_ASSERT_NULL(instance);

    lv_obj_t* obj        = lv_event_get_current_target(event);
    lv_event_code_t code = lv_event_get_code(event);

    if (code != LV_EVENT_CLICKED) {
        return;
    }

    USBSerial.print("HomeMenu -> ");
    M5.Speaker.playWav((const uint8_t*)ResourcePool::GetWav("select_0_5s"), ~0u,
                       1, 1);

    /* Same order as MENU_GRID_IMG / grid slots. */
    static const char* GRID_PAGES[] = {
        "Pages/AppPower",
        "Pages/AppIMU",
        "Pages/AppTouch",
        "Pages/AppI2C",
        "Pages/AppMower",
        "Pages/AppBattery",
    };
    static const char* GRID_NAMES[] = {
        "AppPower", "AppIMU", "AppTouch", "AppI2C", "AppMower", "AppBattery",
    };

    for (size_t i = 0; i < HomeMenuView::GRID_COUNT; i++) {
        if (obj == instance->View.ui.imgbtn_list[i]) {
            USBSerial.println(GRID_NAMES[i]);
            instance->_Manager->Replace(GRID_PAGES[i]);
            return;
        }
    }

    if (obj == instance->View.ui.imgbtn_list[HomeMenuView::SYS_INDEX]) {
        USBSerial.println("AppRTC");
        instance->_Manager->Replace("Pages/AppRTC");
    }
}
