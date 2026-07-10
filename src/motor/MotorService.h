#ifndef MOWER_MOTOR_SERVICE_H
#define MOWER_MOTOR_SERVICE_H

#include <Arduino.h>
#include "mower_config.h"

// Global motor control (P1 logic). Safe to poll from main loop on any page.

enum MotorLoadLevel : uint8_t {
    MOTOR_LOAD_OFF = 0,
    MOTOR_LOAD_LIGHT,
    MOTOR_LOAD_MEDIUM,
    MOTOR_LOAD_HEAVY,
    MOTOR_LOAD_STALL_LIKE,
};

struct MotorTelemetry {
    bool ready;
    bool running;
    bool fault;
    int gear;  // GEAR_ECO / NORMAL / TURBO
    int gear_pct;
    int32_t target_raw;
    float speed;
    float current;
    float vin;
    int temp;
    uint8_t err;
    uint8_t sys;
    MotorLoadLevel load;
    int soft_stall_count;
};

namespace MotorService {

bool begin();  // I2C roller init; false if not found (UI still runs)
void poll();   // fault / soft-stall + periodic status print
void handleSerial();

void toggle();
void setGear(int gear);
void nextGear();
void eStop();       // hard stop + fault latch
void clearFault();  // reset stall protect, stay stopped

const MotorTelemetry& telemetry();
bool isReady();
bool isRunning();
bool isFault();
int gear();
const char* gearName(int g);
const char* loadName(MotorLoadLevel lv);

}  // namespace MotorService

#endif
