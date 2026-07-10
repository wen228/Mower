#ifndef __APPMOWER_MODEL_H
#define __APPMOWER_MODEL_H

#include "motor/MotorService.h"

namespace Page {

class AppMowerModel {
   public:
    void Init();
    const MotorTelemetry& Telem() const;
};

}  // namespace Page

#endif
