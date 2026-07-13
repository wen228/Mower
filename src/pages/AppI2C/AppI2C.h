#ifndef __APPI2C_PRESENTER_H
#define __APPI2C_PRESENTER_H

#include "AppI2CView.h"
#include "AppI2CModel.h"

namespace Page {

class AppI2C : public PageBase {
public:
    AppI2C();
    virtual ~AppI2C();

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
    void runCheckAndShow();
    void AttachEvent(lv_obj_t* obj, lv_event_code_t code);
    static void onEvent(lv_event_t* event);

    AppI2CView View;
    AppI2CModel Model;
};

}  // namespace Page

#endif
