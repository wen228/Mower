/**
 * Mower — app-layer motor business (CoreS3 + Unit Roller I2C)
 * Twin of CLI Mower/; UI uses g_mower + Mower_poll / Mower_handleSerial.
 *
 * config: config_mower.h + config_battery.h
 */
#ifndef MOWER_H
#define MOWER_H

#include <Arduino.h>
#include <cstdint>

#include "config_mower.h"
#include "config_battery.h"
#include "unit_rolleri2c.hpp"

class Mower {
public:
    enum class Load : uint8_t {
        Off = 0,
        Light,
        Medium,
        Heavy,
        StallLike,
    };

    struct Status {
        bool ready;
        bool running;
        bool fault;
        int gear;
        int32_t target_raw;
        int32_t cmd_raw;
        float speed;
        float current;  // mA post-scale
        float vin;      // volts
        int temp;
        uint8_t err;
        uint8_t sys;
        Load load;
        bool ramping;
        int soft_stall_count;
        float batt_power_w; 
        float batt_energy_mwh;
        float batt_soc_pct;
        float batt_used_mah;
        bool batt_low;
    };

    bool begin();
    void update();

    void start();
    void stop(const char* reason = nullptr);
    void toggle();
    void setGear(int gear);
    void eStop();
    void clearFault();
    void resetEnergy();

    Status status() const;

    static const char* gearName(int g);
    static int gearPct(int g);
    static const char* loadName(Load lv);

private:
    UnitRollerI2C roller_;

    bool ready_   = false;
    bool running_ = false;
    bool fault_   = false;
    bool mode_ok_ = false;
    int gear_     = GEAR_ECO;
    int32_t target_ = SPEED_GEAR_ECO;
    int32_t cmd_  = 0;
    int soft_stall_ = 0;

    float speed_   = 0;
    float current_ = 0;
    float vin_     = 0;
    int temp_      = 0;
    uint8_t err_   = 0;
    uint8_t sys_   = 0;
    Load load_     = Load::Off;

    float batt_power_w_    = 0;
    float batt_energy_mwh_ = 0;
    float batt_used_mah_   = 0;
    float batt_soc_pct_    = 100.0f;
    bool batt_low_ = false;

    int32_t last_rgb_ = -1;

    void ensureSpeedMode();
    void rampTick();
    bool isRamping() const;
    Load classifyLoad(int32_t speed_rb, int32_t current_rb) const;
    void updateRgb();
    void updatePowerAndBattery(float dt_s);
    static int32_t speedForGear(int g);
};

extern Mower g_mower;
void Mower_poll();
void Mower_handleSerial();

#endif  // MOWER_H
