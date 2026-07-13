#include "AppSD.h"

#include <cstdio>
#include <cstring>

#include "sd/SdLogger.h"

using namespace Page;

AppSD::AppSD() : timer(nullptr) {
}

AppSD::~AppSD() {
}

void AppSD::onCustomAttrConfig() {
    LV_LOG_USER(__func__);
}

void AppSD::onViewLoad() {
    LV_LOG_USER(__func__);
    View.Create(_root);
    mode_ = ModeBrowse;

    if (Model.IsSDCardExist()) {
        lv_label_set_text(View.ui.label_notice, "Initialize SD Card...");
        Model.SDInit();
    }

    AttachEvent(View.ui.imgbtn_home, LV_EVENT_CLICKED);
    AttachEvent(View.ui.imgbtn_next, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_top_center, LV_EVENT_CLICKED);
    AttachEvent(View.ui.btn_log, LV_EVENT_CLICKED);

    SetBrowseChrome(true);
    lv_label_set_text(View.ui.label_log_btn,
                      g_sd_logger.isRecording() ? "STOP" : "REC");
    lv_obj_set_style_bg_color(View.ui.btn_log,
                              g_sd_logger.isRecording()
                                  ? lv_color_hex(0xCC2222)
                                  : lv_color_hex(0x2E8B57),
                              0);
    lv_label_set_text(View.ui.label_log, g_sd_logger.statusText());
}

void AppSD::onViewDidLoad() {
    LV_LOG_USER(__func__);
}

void AppSD::onViewWillAppear() {
    LV_LOG_USER(__func__);
    timer = lv_timer_create(onTimerUpdate, 200, this);
}

void AppSD::onViewDidAppear() {
    LV_LOG_USER(__func__);
}

void AppSD::onViewWillDisappear() {
    LV_LOG_USER(__func__);
}

void AppSD::onViewDidDisappear() {
    LV_LOG_USER(__func__);
    lv_timer_del(timer);
}

void AppSD::onViewUnload() {
    LV_LOG_USER(__func__);

    View.Delete();
    scan_flag = true;
    mode_     = ModeBrowse;
    Model.SDDeinit();
}

void AppSD::onViewDidUnload() {
    LV_LOG_USER(__func__);
}

void AppSD::AttachEvent(lv_obj_t* obj, lv_event_code_t code) {
    lv_obj_set_user_data(obj, this);
    lv_obj_add_event_cb(obj, onEvent, code, this);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void AppSD::SetBrowseChrome(bool browse) {
    if (browse) {
        lv_obj_clear_flag(View.ui.btn_log, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(View.ui.label_log, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(View.ui.btn_log, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(View.ui.label_log, LV_OBJ_FLAG_HIDDEN);
    }
}

void AppSD::ListSDCard(fs::FS& fs, const char* dirname, uint8_t levels) {
    File root = fs.open(dirname);
    if (!root) {
        lv_obj_clean(View.ui.file_list);
        lv_label_set_text(View.ui.label_notice,
                          "Failed to open root directory");
        lv_obj_clear_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (!root.isDirectory()) {
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            lv_obj_t* label = lv_label_create(View.ui.file_list);
            lv_label_set_text_fmt(label, "DIR: %-31.31s", file.name());
            if (levels) {
                ListSDCard(fs, file.path(), levels - 1);
            }
        } else {
            const char* name = file.name();
            const char* base = strrchr(name, '/');
            base             = base ? base + 1 : name;

            /* Mower_MMDD_HHMMSS.csv (or legacy mower_XX.csv) → clickable */
            int mon = 0, day = 0, hh = 0, mi = 0, se = 0;
            unsigned idx = 0;
            const bool is_mower =
                (sscanf(base, "Mower_%2d%2d_%2d%2d%2d.csv", &mon, &day, &hh,
                        &mi, &se) == 5) ||
                (sscanf(base, "mower_%u.csv", &idx) == 1 && idx >= 1 &&
                 idx <= 99);
            if (is_mower) {
                char caption[48];
                /* First token = filename for onFileClick. */
                snprintf(caption, sizeof(caption), "%s %dKB", base,
                         (int)(file.size() / 1024));
                lv_obj_t* btn =
                    lv_list_add_btn(View.ui.file_list, NULL, caption);
                lv_obj_add_event_cb(btn, onFileClick, LV_EVENT_CLICKED, this);
            } else {
                lv_obj_t* label = lv_label_create(View.ui.file_list);
                lv_label_set_text_fmt(label, "  F: %-24.24s %05dKB", base,
                                      (int)(file.size() / 1024));
            }
        }
        file = root.openNextFile();
    }
}

void AppSD::EnterCsvView(const char* path) {
    if (g_sd_logger.isRecording()) {
        lv_label_set_text(View.ui.label_notice, "Stop REC before open");
        lv_obj_clear_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (!Model.GetInitFlag() && !Model.SDInit()) {
        lv_label_set_text(View.ui.label_notice, "SD not ready");
        lv_obj_clear_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (!Model.ReadCsvTail50(path)) {
        lv_label_set_text(View.ui.label_notice, "Open CSV failed");
        lv_obj_clear_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    strncpy(view_path_, path, sizeof(view_path_) - 1);
    view_path_[sizeof(view_path_) - 1] = '\0';
    mode_                              = ModeView;
    scan_flag                          = false;
    SetBrowseChrome(false);

    lv_obj_clean(View.ui.file_list);
    lv_obj_add_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);

    /* title + header + rows (data lives in Model, not on stack) */
    lv_obj_t* t = lv_label_create(View.ui.file_list);
    lv_label_set_text_fmt(t, "%s  (tap title=back)", path);
    lv_obj_t* h = lv_label_create(View.ui.file_list);
    lv_label_set_text(h, "t M:S  tgt  rpm    I  run fault");

    const int n = Model.viewLineCount();
    if (n == 0) {
        lv_obj_t* empty = lv_label_create(View.ui.file_list);
        lv_label_set_text(empty, "(no data rows)");
    } else {
        for (int i = 0; i < n; i++) {
            lv_obj_t* lab = lv_label_create(View.ui.file_list);
            lv_label_set_text(lab, Model.viewLine(i));
        }
    }
}

void AppSD::ExitCsvView() {
    mode_     = ModeBrowse;
    scan_flag = true;
    SetBrowseChrome(true);
    lv_obj_clean(View.ui.file_list);
}

void AppSD::Update() {
    if (mode_ == ModeView) {
        /* static view — only keep log status text if needed; chrome hidden */
        return;
    }

    lv_label_set_text(View.ui.label_log, g_sd_logger.statusText());
    lv_label_set_text(View.ui.label_log_btn,
                      g_sd_logger.isRecording() ? "STOP" : "REC");
    lv_obj_set_style_bg_color(View.ui.btn_log,
                              g_sd_logger.isRecording()
                                  ? lv_color_hex(0xCC2222)
                                  : lv_color_hex(0x2E8B57),
                              0);

    if (Model.IsSDCardExist()) {
        if (!Model.GetInitFlag()) {
            static uint32_t last_try_ms = 0;
            const uint32_t now = millis();
            if (now - last_try_ms < 1500) {
                return;
            }
            last_try_ms = now;

            lv_obj_clean(View.ui.file_list);
            lv_label_set_text(View.ui.label_notice,
                              "Reinitialize the SD Card...");
            lv_obj_clear_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);
            Model.SDDeinit();
            if (!Model.SDInit()) {
                lv_label_set_text(View.ui.label_notice,
                                  "SD Card initialization failed!\r\nPlease "
                                  "try reinsert SD card...");
            } else {
                scan_flag = true;
            }
        } else {
            lv_label_set_text(View.ui.label_notice, "SD Card is ready");
            lv_obj_add_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);
            if (scan_flag) {
                lv_obj_clean(View.ui.file_list);
                ListSDCard(SD, "/", 2);
                scan_flag = false;
            }
        }
    } else {
        Model.ClearInitFlag();
        scan_flag = true;
        lv_obj_clean(View.ui.file_list);
        lv_label_set_text(View.ui.label_notice, "Please insert SD card...");
        lv_obj_clear_flag(View.ui.label_notice, LV_OBJ_FLAG_HIDDEN);
    }
}

void AppSD::onTimerUpdate(lv_timer_t* timer) {
    AppSD* instance = (AppSD*)timer->user_data;
    instance->Update();
}

void AppSD::onFileClick(lv_event_t* event) {
    AppSD* instance = (AppSD*)lv_event_get_user_data(event);
    if (!instance || instance->mode_ != ModeBrowse) {
        return;
    }
    if (lv_event_get_code(event) != LV_EVENT_CLICKED) {
        return;
    }

    lv_obj_t* btn = lv_event_get_current_target(event);
    /* Prefer list btn; if click landed on child, walk up. */
    while (btn && lv_obj_get_parent(btn) != instance->View.ui.file_list) {
        btn = lv_obj_get_parent(btn);
    }
    if (!btn) {
        return;
    }

    const char* txt =
        lv_list_get_btn_text(instance->View.ui.file_list, btn);
    if (!txt) {
        return;
    }

    char base[32] = {0};
    if (sscanf(txt, "%31s", base) != 1) {
        return;
    }
    int mon = 0, day = 0, hh = 0, mi = 0, se = 0;
    unsigned idx = 0;
    const bool is_mower =
        (sscanf(base, "Mower_%2d%2d_%2d%2d%2d.csv", &mon, &day, &hh, &mi,
                &se) == 5) ||
        (sscanf(base, "mower_%u.csv", &idx) == 1 && idx >= 1 && idx <= 99);
    if (!is_mower) {
        return;
    }

    M5.Speaker.playWav((const uint8_t*)ResourcePool::GetWav("select_0_5s"), ~0u,
                       1, 1);

    char path[36];
    snprintf(path, sizeof(path), "/%s", base);
    instance->EnterCsvView(path);
}

void AppSD::onEvent(lv_event_t* event) {
    AppSD* instance = (AppSD*)lv_event_get_user_data(event);
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
        USBSerial.print("AppSD -> AppPower\r\n");
        instance->_Manager->Replace("Pages/AppPower");
        return;
    }

    /* Title area: back from view, or rescan in browse */
    if (obj == instance->View.ui.btn_top_center) {
        if (instance->mode_ == ModeView) {
            instance->ExitCsvView();
        } else {
            instance->scan_flag = true;
        }
        return;
    }

    if (instance->mode_ == ModeBrowse && obj == instance->View.ui.btn_log) {
        g_sd_logger.toggle();
        instance->scan_flag = true;
        lv_label_set_text(instance->View.ui.label_log,
                          g_sd_logger.statusText());
        lv_label_set_text(instance->View.ui.label_log_btn,
                          g_sd_logger.isRecording() ? "STOP" : "REC");
        lv_obj_set_style_bg_color(instance->View.ui.btn_log,
                                  g_sd_logger.isRecording()
                                      ? lv_color_hex(0xCC2222)
                                      : lv_color_hex(0x2E8B57),
                                  0);
    }
}
