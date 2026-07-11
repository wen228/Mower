/**
 * Mower — app-layer motor business (CoreS3 + Unit Roller I2C)
 *
 * Same class as CLI Mower/ project. UI uses global g_mower + Mower_poll /
 * Mower_handleSerial (USBSerial).
 *
 * Actions: start/stop/toggle/setGear/eStop/clearFault
 * Per-loop: update()  (ramp + telemetry + stall + status RGB)
 * Read:     status()
 */
#ifndef MOWER_H
#define MOWER_H

#include <Arduino.h>
#include <cstdint>

#include "mower_config.h"
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
        int gear;  // 1=Eco 2=Normal 3=Turbo
        int32_t target_raw;
        int32_t cmd_raw;
        float speed;
        float current;
        float vin;
        int temp;
        uint8_t err;
        uint8_t sys;
        Load load;
        bool ramping;
        int soft_stall_count;
    };

    bool begin();
    void update();

    void start();
    void stop(const char* reason = nullptr);
    void toggle();
    void setGear(int gear);
    void eStop();  // hard stop + fault latch (UI button / serial 'e')
    void clearFault();

    Status status() const;

    static const char* gearName(int g);
    static int gearPct(int g);
    static const char* loadName(Load lv);

private:
    UnitRollerI2C roller_;

    bool ready_    = false;
    bool running_  = false;
    bool fault_    = false;
    bool mode_ok_  = false;
    int gear_      = GEAR_ECO;
    int32_t target_ = SPEED_GEAR_ECO;
    int32_t cmd_   = 0;
    int soft_stall_ = 0;

    float speed_   = 0;
    float current_ = 0;
    float vin_     = 0;
    int temp_      = 0;
    uint8_t err_   = 0;
    uint8_t sys_   = 0;
    Load load_     = Load::Off;

    int32_t last_rgb_ = -1;

    void ensureSpeedMode();
    void rampTick();
    bool isRamping() const;
    Load classifyLoad(int32_t speed_rb, int32_t current_rb) const;
    void updateRgb();
    static int32_t speedForGear(int g);
};

// Global instance for App + main loop (one Roller).
extern Mower g_mower;

// Rate-limited update (~LOOP_PERIOD_MS) + optional status print.
void Mower_poll();
// USBSerial CLI: t/1/2/3/e/r/s/h
void Mower_handleSerial();

#endif  // MOWER_H
