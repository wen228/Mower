#ifndef __APPBATTERY_MODEL_H
#define __APPBATTERY_MODEL_H

#include "motor/Mower.h"

namespace Page {

class AppBatteryModel {
   public:
    void Init();
    Mower::Status status() const;
    void resetEnergy();
};

}  // namespace Page

#endif
