#ifndef __APPMOWER_MODEL_H
#define __APPMOWER_MODEL_H

#include "motor/Mower.h"

namespace Page {

class AppMowerModel {
   public:
    void Init();
    Mower::Status status() const;
};

}  // namespace Page

#endif
