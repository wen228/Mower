#ifndef MOWER_CONFIG_BATTERY_H
#define MOWER_CONFIG_BATTERY_H

// Battery / energy estimate (app-layer coulomb count, not a true BMS).
// Align with Mower/include/config_battery.h

#define BATTERY_CAP_MAH      20.0f
#define BATTERY_LOW_SOC_PCT  20.0f
#define BATTERY_LOW_LATCH    1  // 1: stop + fault on low SOC; 0: flag only

#endif  // MOWER_CONFIG_BATTERY_H
