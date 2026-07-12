#include "AppSDModel.h"

#include "sd/SdMount.h"

using namespace Page;

bool AppSDModel::SDInit() {
    if (init_flag) {
        return true;
    }
    init_flag = SdMount();
    return init_flag;
}

void AppSDModel::SDDeinit() {
    if (!init_flag) {
        return;
    }
    SdUnmount();
    init_flag = false;
}

bool AppSDModel::GetInitFlag() {
    return init_flag;
}

void AppSDModel::ClearInitFlag() {
    /* Card removed: drop our ref without assuming SD still valid. */
    if (init_flag) {
        SdUnmount();
        init_flag = false;
    }
}

bool AppSDModel::IsSDCardExist() {
    return SdCardPresent();
}
