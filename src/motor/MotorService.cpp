#include "MotorService.h"

#include <Wire.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cmath>

#include "unit_rolleri2c.hpp"

namespace MotorService {

static UnitRollerI2C roller;
static bool ready           = false;
static bool motor_running   = false;
static bool fault_latched   = false;
static int32_t target_speed = SPEED_GEAR_ECO;
static int gear_idx         = GEAR_ECO;
static int soft_stall_count = 0;
static uint32_t last_print_ms = 0;
static uint32_t last_poll_ms  = 0;
static MotorTelemetry telem;

static void logln(const char* msg) {
    USBSerial.println(msg);
}

static void logf(const char* fmt, ...) {
    char buf[192];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    USBSerial.print(buf);
}

const char* gearName(int g) {
    switch (g) {
        case GEAR_ECO: return "Eco";
        case GEAR_NORMAL: return "Normal";
        case GEAR_TURBO: return "Turbo";
        default: return "?";
    }
}

static int gearPct(int g) {
    switch (g) {
        case GEAR_ECO: return SPEED_PCT_ECO;
        case GEAR_NORMAL: return SPEED_PCT_NORMAL;
        case GEAR_TURBO: return SPEED_PCT_TURBO;
        default: return 0;
    }
}

static int32_t speedForGear(int g) {
    switch (g) {
        case GEAR_ECO: return SPEED_GEAR_ECO;
        case GEAR_NORMAL: return SPEED_GEAR_NORMAL;
        case GEAR_TURBO: return SPEED_GEAR_TURBO;
        default: return SPEED_GEAR_ECO;
    }
}

const char* loadName(MotorLoadLevel lv) {
    switch (lv) {
        case MOTOR_LOAD_OFF: return "OFF";
        case MOTOR_LOAD_LIGHT: return "LIGHT";
        case MOTOR_LOAD_MEDIUM: return "MEDIUM";
        case MOTOR_LOAD_HEAVY: return "HEAVY";
        case MOTOR_LOAD_STALL_LIKE: return "STALL?";
        default: return "?";
    }
}

static void printHelp() {
    logln("---- AppMower commands ----");
    logln("  t/space : toggle run/stop");
    logln("  1       : Eco    30%");
    logln("  2       : Normal 70%");
    logln("  3       : Turbo  100%");
    logln("  e       : E-STOP (latch fault)");
    logln("  r       : reset fault (stay stopped)");
    logln("  s       : status once");
    logln("  h       : help");
}

static bool rollerApplySpeedMode(int32_t speed) {
    if (!ready) {
        return false;
    }
    roller.setOutput(0);
    delay(20);
    roller.setMode(ROLLER_MODE_SPEED);
    roller.setSpeed(speed);
    roller.setSpeedMaxCurrent(SPEED_MAX_CURRENT);
    if (motor_running && !fault_latched) {
        roller.setOutput(1);
    }
    return true;
}

static void motorStop(const char* reason) {
    if (ready) {
        roller.setOutput(0);
    }
    motor_running    = false;
    soft_stall_count = 0;
    logf("[STOP] %s\n", reason ? reason : "");
}

static void motorStart() {
    if (!ready) {
        logln("[START] blocked: roller not ready");
        return;
    }
    if (fault_latched) {
        logln("[START] blocked: fault latched, press 'r' / Reset");
        return;
    }
    rollerApplySpeedMode(target_speed);
    roller.setOutput(1);
    motor_running    = true;
    soft_stall_count = 0;
    logf("[START] %s %d%% raw=%ld\n", gearName(gear_idx), gearPct(gear_idx),
         (long)target_speed);
}

static MotorLoadLevel classifyLoad(int32_t speed_rb, int32_t current_rb) {
    if (!motor_running) {
        return MOTOR_LOAD_OFF;
    }

    const float n_act = fabsf((float)speed_rb);
    const float n_tgt = fabsf((float)target_speed);
    const float i_act = fabsf((float)current_rb);

    if (n_tgt > 1.0f && i_act >= (float)LOAD_STALL_MIN_I &&
        (n_act / n_tgt) < LOAD_STALL_SPEED_RATIO) {
        return MOTOR_LOAD_STALL_LIKE;
    }
    if (i_act <= (float)LOAD_LIGHT_MAX_I) {
        return MOTOR_LOAD_LIGHT;
    }
    if (i_act <= (float)LOAD_MEDIUM_MAX_I) {
        return MOTOR_LOAD_MEDIUM;
    }
    return MOTOR_LOAD_HEAVY;
}

static void updateTelemetry(float speed, float current, float vin, int temp,
                            MotorLoadLevel load, uint8_t err, uint8_t sys) {
    telem.ready           = ready;
    telem.running         = motor_running;
    telem.fault           = fault_latched;
    telem.gear            = gear_idx;
    telem.gear_pct        = gearPct(gear_idx);
    telem.target_raw      = target_speed;
    telem.speed           = speed;
    telem.current         = current;
    telem.vin             = vin;
    telem.temp            = temp;
    telem.err             = err;
    telem.sys             = sys;
    telem.load            = load;
    telem.soft_stall_count = soft_stall_count;
}

static void printStatus(float speed, float current, float vin, int temp,
                        MotorLoadLevel load, uint8_t err, uint8_t sys) {
    logf(
        "run=%d gear=%s(%d%%) tgt=%ld spd=%.1f I=%.1f Vin=%.2f T=%d load=%s "
        "err=%u sys=%u fault=%d soft_stall=%d\n",
        motor_running ? 1 : 0, gearName(gear_idx), gearPct(gear_idx),
        (long)target_speed, speed, current, vin, temp, loadName(load), err, sys,
        fault_latched ? 1 : 0, soft_stall_count);
}

bool begin() {
    memset(&telem, 0, sizeof(telem));
    gear_idx      = GEAR_ECO;
    target_speed  = SPEED_GEAR_ECO;
    motor_running = false;
    fault_latched = false;

    logln("==== MotorService (AppMower) ====");
    logf("PortA I2C SDA=%d SCL=%d addr=0x%02X\n", ROLLER_I2C_SDA,
         ROLLER_I2C_SCL, ROLLER_I2C_ADDR);

    if (!roller.begin(&Wire, ROLLER_I2C_ADDR, ROLLER_I2C_SDA, ROLLER_I2C_SCL,
                      ROLLER_I2C_HZ)) {
        logln("[ERR] Roller I2C not found. UI/serial still work.");
        ready = false;
        updateTelemetry(0, 0, 0, 0, MOTOR_LOAD_OFF, 0, 0);
        printHelp();
        return false;
    }

    ready = true;
    roller.setStallProtection(1);
    roller.setOutput(0);
    roller.setMode(ROLLER_MODE_SPEED);
    roller.setSpeed(target_speed);
    roller.setSpeedMaxCurrent(SPEED_MAX_CURRENT);
    logf("[OK] stall_protect=%u mode=SPEED maxI=%d\n",
         roller.getStallProtection(), SPEED_MAX_CURRENT);
#if MOWER_PWR_PROFILE == 0
    logln("Ready [5V profile].");
#else
    logln("Ready [12V profile].");
#endif
    logf("SPEED_FULL=%ld Eco=%ld Nor=%ld Tur=%ld\n", (long)SPEED_FULL,
         (long)SPEED_GEAR_ECO, (long)SPEED_GEAR_NORMAL, (long)SPEED_GEAR_TURBO);
    printHelp();
    updateTelemetry(0, 0, 0, 0, MOTOR_LOAD_OFF, 0, 0);
    return true;
}

void toggle() {
    if (motor_running) {
        motorStop("user");
    } else {
        motorStart();
    }
}

void setGear(int g) {
    if (g < GEAR_ECO || g > GEAR_TURBO) {
        g = GEAR_ECO;
    }
    gear_idx     = g;
    target_speed = speedForGear(gear_idx);
    logf("[GEAR] %s %d%% raw=%ld\n", gearName(gear_idx), gearPct(gear_idx),
         (long)target_speed);
    if (motor_running && !fault_latched && ready) {
        rollerApplySpeedMode(target_speed);
        roller.setOutput(1);
        motor_running = true;
    }
    telem.gear       = gear_idx;
    telem.gear_pct   = gearPct(gear_idx);
    telem.target_raw = target_speed;
}

void nextGear() {
    int g = gear_idx + 1;
    if (g > GEAR_TURBO) {
        g = GEAR_ECO;
    }
    setGear(g);
}

void eStop() {
    if (ready) {
        roller.setOutput(0);
    }
    motor_running    = false;
    fault_latched    = true;
    soft_stall_count = 0;
    logln("[E-STOP] latched. Press 'r' / Reset to clear.");
    telem.running = false;
    telem.fault   = true;
}

void clearFault() {
    if (ready) {
        roller.setOutput(0);
        roller.resetStalledProtect();
        delay(50);
        roller.setStallProtection(1);
    }
    motor_running    = false;
    fault_latched    = false;
    soft_stall_count = 0;
    logln("[FAULT] cleared; stall protect re-enabled. Press 't' to start.");
    telem.running = false;
    telem.fault   = false;
}

void poll() {
    const uint32_t now = millis();
    if (now - last_poll_ms < (uint32_t)LOOP_PERIOD_MS) {
        return;
    }
    last_poll_ms = now;

    int32_t speed_rb   = 0;
    int32_t current_rb = 0;
    int32_t vin_raw    = 0;
    int temp           = 0;
    uint8_t err        = 0;
    uint8_t sys        = 0;

    if (ready) {
        speed_rb   = roller.getSpeedReadback();
        current_rb = roller.getCurrentReadback();
        vin_raw    = roller.getVin();
        temp       = roller.getTemp();
        err        = roller.getErrorCode();
        sys        = roller.getSysStatus();
    }

    const float speed   = speed_rb / SCALE_SPEED_DIV;
    const float current = current_rb / SCALE_CURRENT_DIV;
    const float vin     = vin_raw / SCALE_VIN_DIV;

    if (ready) {
        if (err == ERR_JAM || sys == SYS_ERROR) {
            if (!fault_latched) {
                fault_latched = true;
                motorStop(err == ERR_JAM ? "HW jam (err=2)" : "sys error");
                logf("[FAULT] err=%u sys=%u\n", err, sys);
            }
        }
        if (err == ERR_OVERVOLTAGE) {
            if (!fault_latched) {
                fault_latched = true;
                motorStop("overvoltage (err=1)");
            }
        }
    }

    MotorLoadLevel load = classifyLoad(speed_rb, current_rb);
    if (motor_running && load == MOTOR_LOAD_STALL_LIKE) {
        soft_stall_count++;
        if (soft_stall_count >= LOAD_STALL_COUNT_MAX) {
            fault_latched = true;
            motorStop("soft stall (I high, speed low)");
        }
    } else if (motor_running) {
        soft_stall_count = 0;
    }

    updateTelemetry(speed, current, vin, temp, load, err, sys);

    if (now - last_print_ms >= (uint32_t)PRINT_PERIOD_MS) {
        last_print_ms = now;
        if (motor_running || fault_latched) {
            printStatus(speed, current, vin, temp, load, err, sys);
        }
    }
}

void handleSerial() {
    while (USBSerial.available()) {
        char c = (char)USBSerial.read();
        if (c == '\r' || c == '\n') {
            continue;
        }
        switch (c) {
            case 't':
            case 'T':
            case ' ':
                toggle();
                break;
            case '1':
                setGear(GEAR_ECO);
                break;
            case '2':
                setGear(GEAR_NORMAL);
                break;
            case '3':
                setGear(GEAR_TURBO);
                break;
            case 'e':
            case 'E':
                eStop();
                break;
            case 'r':
            case 'R':
                clearFault();
                break;
            case 's':
            case 'S':
                last_print_ms = 0;
                printStatus(telem.speed, telem.current, telem.vin, telem.temp,
                            telem.load, telem.err, telem.sys);
                break;
            case 'h':
            case 'H':
            case '?':
                printHelp();
                break;
            default:
                break;
        }
    }
}

const MotorTelemetry& telemetry() {
    return telem;
}

bool isReady() {
    return ready;
}

bool isRunning() {
    return motor_running;
}

bool isFault() {
    return fault_latched;
}

int gear() {
    return gear_idx;
}

}  // namespace MotorService
