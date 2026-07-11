#include "AppMowerModel.h"

using namespace Page;

void AppMowerModel::Init() {
    /* g_mower.begin() is called once from App_Init */
}

Mower::Status AppMowerModel::status() const {
    return g_mower.status();
}
