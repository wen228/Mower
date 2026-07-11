#include "AppBatteryModel.h"

using namespace Page;

void AppBatteryModel::Init() {
    /* g_mower.begin() from App_Init */
}

Mower::Status AppBatteryModel::status() const {
    return g_mower.status();
}

void AppBatteryModel::resetEnergy() {
    g_mower.resetEnergy();
}
