#ifndef __APPBATTERY_PRESENTER_H
#define __APPBATTERY_PRESENTER_H

#include "AppBatteryView.h"
#include "AppBatteryModel.h"

namespace Page {

class AppBattery : public PageBase {
   public:
    AppBattery();
    virtual ~AppBattery();

    virtual void onCustomAttrConfig();
    virtual void onViewLoad();
    virtual void onViewDidLoad();
    virtual void onViewWillAppear();
    virtual void onViewDidAppear();
    virtual void onViewWillDisappear();
    virtual void onViewDidDisappear();
    virtual void onViewUnload();
    virtual void onViewDidUnload();

   private:
    void Update();
    void AttachEvent(lv_obj_t* obj, lv_event_code_t code);
    static void onTimerUpdate(lv_timer_t* timer);
    static void onEvent(lv_event_t* event);

   private:
    AppBatteryView View;
    AppBatteryModel Model;
    lv_timer_t* timer;
};

}  // namespace Page

#endif
