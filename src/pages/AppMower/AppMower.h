#ifndef __APPMOWER_PRESENTER_H
#define __APPMOWER_PRESENTER_H

#include "AppMowerView.h"
#include "AppMowerModel.h"

namespace Page {

class AppMower : public PageBase {
   public:
    AppMower();
    virtual ~AppMower();

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
    void AttachEvent(lv_obj_t* obj) {
        AttachEvent(obj, LV_EVENT_ALL);
    }
    void AttachEvent(lv_obj_t* obj, lv_event_code_t code);
    static void onTimerUpdate(lv_timer_t* timer);
    static void onEvent(lv_event_t* event);

   private:
    AppMowerView View;
    AppMowerModel Model;
    lv_timer_t* timer;
};

}  // namespace Page

#endif
