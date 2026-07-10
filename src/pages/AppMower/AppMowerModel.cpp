#include "AppMowerModel.h"

using namespace Page;

void AppMowerModel::Init() {
    /* MotorService::begin() is called once from App_Init */
}

const MotorTelemetry& AppMowerModel::Telem() const {
    return MotorService::telemetry();
}
