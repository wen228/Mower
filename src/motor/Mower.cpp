#include "motor/Mower.h"

#include <Wire.h>
#include <cmath>
#include <cstdio>

#define MOWER_LOG USBSerial

// RGB565 for UnitRollerI2C::setRGB (same layout as M5 TFT_*).
// Avoid bare RGB_BRIGHTNESS — ESP32 pins_arduino.h already #defines it.
static const int32_t MOWER_RGB_IDLE  = 0x001F;  // blue  — stopped, OK
static const int32_t MOWER_RGB_RUN   = 0x07E0;  // green — running / ramping
static const int32_t MOWER_RGB_LOAD  = 0xFFE0;  // yellow — Medium / Heavy
static const int32_t MOWER_RGB_FAULT = 0xF800;  // red — fault latched
static const uint8_t MOWER_RGB_BRI   = 80;

Mower g_mower;

const char* Mower::gearName(int g)
{
    switch (g) {
        case GEAR_ECO: return "Eco";
        case GEAR_NORMAL: return "Normal";
        case GEAR_TURBO: return "Turbo";
        default: return "?";
    }
}

int Mower::gearPct(int g)
{
    switch (g) {
        case GEAR_ECO: return SPEED_PCT_ECO;
        case GEAR_NORMAL: return SPEED_PCT_NORMAL;
        case GEAR_TURBO: return SPEED_PCT_TURBO;
        default: return 0;
    }
}

const char* Mower::loadName(Load lv)
{
    switch (lv) {
        case Load::Off: return "OFF";
        case Load::Light: return "LIGHT";
        case Load::Medium: return "MEDIUM";
        case Load::Heavy: return "HEAVY";
        case Load::StallLike: return "STALL?";
        default: return "?";
    }
}

int32_t Mower::speedForGear(int g)
{
    switch (g) {
        case GEAR_ECO: return SPEED_GEAR_ECO;
        case GEAR_NORMAL: return SPEED_GEAR_NORMAL;
        case GEAR_TURBO: return SPEED_GEAR_TURBO;
        default: return SPEED_GEAR_ECO;
    }
}

bool Mower::begin()
{
    ready_ = false;
    if (!roller_.begin(&Wire, ROLLER_I2C_ADDR, ROLLER_I2C_SDA, ROLLER_I2C_SCL,
                       ROLLER_I2C_HZ)) {
        mode_ok_ = false;
        MOWER_LOG.println("[ERR] Roller I2C not found. UI/serial still work.");
        return false;
    }

    roller_.setStallProtection(1);
    roller_.setOutput(0);
    roller_.setMode(ROLLER_MODE_SPEED);
    roller_.setSpeed(0);
    roller_.setSpeedMaxCurrent(SPEED_MAX_CURRENT);
    cmd_     = 0;
    mode_ok_ = true;
    ready_   = true;

    roller_.setRGBBrightness(MOWER_RGB_BRI);
    roller_.setRGBMode(ROLLER_RGB_MODE_USER_DEFINED);
    last_rgb_ = -1;
    resetEnergy();
    updateRgb();

    MOWER_LOG.printf("[OK] Mower ready  maxI=%d batt=%.0fmAh\n",
                     SPEED_MAX_CURRENT, (double)BATTERY_CAP_MAH);
    return true;
}

void Mower::ensureSpeedMode()
{
    if (!ready_ || mode_ok_) {
        return;
    }
    roller_.setOutput(0);
    delay(20);
    roller_.setMode(ROLLER_MODE_SPEED);
    roller_.setSpeedMaxCurrent(SPEED_MAX_CURRENT);
    roller_.setSpeed(0);
    cmd_     = 0;
    mode_ok_ = true;
}

void Mower::stop(const char* reason)
{
    if (ready_) {
        roller_.setOutput(0);
    }
    running_    = false;
    cmd_        = 0;
    soft_stall_ = 0;
    /* Keep reason for AppMower tips when already latched (log text unchanged). */
    if (fault_ && reason && reason[0] != '\0') {
        snprintf(fault_reason_, sizeof(fault_reason_), "%s", reason);
    }
    if (ready_) {
        updateRgb();
    }
    MOWER_LOG.printf("[STOP] %s\n", reason ? reason : "");
}

void Mower::start()
{
    if (!ready_) {
        MOWER_LOG.println("[START] blocked: roller not ready");
        return;
    }
    if (fault_) {
        MOWER_LOG.println("[START] blocked: fault latched, press 'r' to clear");
        return;
    }
    ensureSpeedMode();
    cmd_ = 0;
    roller_.setSpeed(0);
    roller_.setSpeedMaxCurrent(SPEED_MAX_CURRENT);
    roller_.setOutput(1);
    running_    = true;
    soft_stall_ = 0;
    updateRgb();
    MOWER_LOG.printf("[START] ramp to %s %d%% tgt=%ld\n", gearName(gear_),
                     gearPct(gear_), (long)target_);
}

void Mower::toggle()
{
    if (running_) {
        stop("user");
    } else {
        start();
    }
}

void Mower::setGear(int g)
{
    if (g < GEAR_ECO || g > GEAR_TURBO) {
        g = GEAR_ECO;
    }
    gear_   = g;
    target_ = speedForGear(gear_);
    MOWER_LOG.printf("[GEAR] %s %d%% tgt=%ld cmd=%ld\n", gearName(gear_),
                     gearPct(gear_), (long)target_, (long)cmd_);
}

void Mower::eStop()
{
    if (ready_) {
        roller_.setOutput(0);
    }
    running_    = false;
    cmd_        = 0;
    soft_stall_ = 0;
    fault_      = true;
    snprintf(fault_reason_, sizeof(fault_reason_), "E-STOP");
    if (ready_) {
        updateRgb();
    }
    MOWER_LOG.println("[E-STOP] latched. Press 'r' / Reset to clear.");
}

void Mower::clearFault()
{
    if (ready_) {
        roller_.setOutput(0);
        roller_.resetStalledProtect();
        delay(50);
        roller_.setStallProtection(1);
    }
    running_    = false;
    cmd_        = 0;
    fault_      = false;
    soft_stall_ = 0;
    mode_ok_    = false;
    fault_reason_[0] = '\0';
    if (ready_) {
        updateRgb();
    }
    MOWER_LOG.println("[FAULT] cleared. Press 't' to start. ('b' = full SOC)");
}

void Mower::resetEnergy()
{
    batt_power_w_     = 0;
    batt_energy_mwh_  = 0;
    batt_used_mah_    = 0;
    batt_soc_pct_     = 100.0f;
    batt_low_ = false;
    MOWER_LOG.printf("[BATT] reset full  cap=%.0fmAh SOC=100%%\n",
                     (double)BATTERY_CAP_MAH);
}

bool Mower::isRamping() const
{
    const int32_t diff = target_ - cmd_;
    const int32_t ad   = (diff >= 0) ? diff : -diff;
    return ad > SPEED_CMD_MIN_EPS;
}

void Mower::rampTick()
{
    if (!ready_ || !running_ || fault_) {
        return;
    }

    const int32_t diff = target_ - cmd_;
    const int32_t ad   = (diff >= 0) ? diff : -diff;

    if (ad <= SPEED_CMD_MIN_EPS) {
        if (cmd_ != target_) {
            cmd_ = target_;
            roller_.setSpeed(cmd_);
        }
        return;
    }

    const int32_t step = (diff > 0) ? SPEED_RAMP_UP_STEP : SPEED_RAMP_DOWN_STEP;
    if (diff > 0) {
        cmd_ += (diff < step) ? diff : step;
    } else {
        cmd_ -= ((-diff) < step) ? (-diff) : step;
    }
    roller_.setSpeed(cmd_);
}

Mower::Load Mower::classifyLoad(int32_t speed_rb, int32_t current_rb) const
{
    if (!running_) {
        return Load::Off;
    }

    const float n_act = fabsf((float)speed_rb);
    const float n_cmd = fabsf((float)cmd_);
    const float i_act = fabsf((float)current_rb);
    // High current + actual speed << cmd → stall-like.
    if (n_cmd > 1.0f && i_act >= (float)LOAD_STALL_MIN_I &&
        (n_act / n_cmd) < LOAD_STALL_SPEED_RATIO) {
        return Load::StallLike;
    }
    if (i_act <= (float)LOAD_LIGHT_MAX_I) {
        return Load::Light;
    }
    if (i_act <= (float)LOAD_MEDIUM_MAX_I) {
        return Load::Medium;
    }
    return Load::Heavy;
}

void Mower::update()
{
    if (!ready_) {
        return;
    }

    rampTick();

    const int32_t speed_rb   = roller_.getSpeedReadback();
    const int32_t current_rb = roller_.getCurrentReadback();
    const int32_t vin_raw    = roller_.getVin();
    temp_ = roller_.getTemp();
    err_  = roller_.getErrorCode();
    sys_  = roller_.getSysStatus();

    speed_   = speed_rb / SCALE_SPEED_DIV;
    current_ = current_rb / SCALE_CURRENT_DIV;
    vin_     = vin_raw / SCALE_VIN_DIV;

    if (err_ == ERR_JAM || sys_ == SYS_ERROR) {
        if (!fault_) {
            fault_ = true;
            stop(err_ == ERR_JAM ? "HW jam (err=2)" : "sys error");
            MOWER_LOG.printf("[FAULT] err=%u sys=%u\n", err_, sys_);
        }
    }
    if (err_ == ERR_OVERVOLTAGE) {
        if (!fault_) {
            fault_ = true;
            stop("overvoltage (err=1)");
        }
    }

    load_ = classifyLoad(speed_rb, current_rb);

    if (running_ && load_ == Load::StallLike && !isRamping()) {
        soft_stall_++;
        if (soft_stall_ >= LOAD_STALL_COUNT_MAX) {
            fault_ = true;
            stop("soft stall (I high, speed low)");
        }
    } else if (running_) {
        soft_stall_ = 0;
    }

    updatePowerAndBattery(LOOP_PERIOD_MS / 1000.0f);
    updateRgb();
}

void Mower::updatePowerAndBattery(float dt_s)
{
    // vin_ volts; current_ mA (post SCALE_*_DIV)
    if (dt_s <= 0.0f) {
        return;
    }

    const float i_ma = fabsf(current_);
    const float v    = fabsf(vin_);

    batt_power_w_ = v * (i_ma / 1000.0f);
    batt_energy_mwh_ += batt_power_w_ * dt_s * 1000.0f / 3600.0f;

    batt_used_mah_ += i_ma * dt_s / 3600.0f;
    if (batt_used_mah_ < 0.0f) {
        batt_used_mah_ = 0.0f;
    }
    if (batt_used_mah_ > BATTERY_CAP_MAH) {
        batt_used_mah_ = BATTERY_CAP_MAH;
    }

    batt_soc_pct_ = 100.0f * (1.0f - batt_used_mah_ / BATTERY_CAP_MAH);
    if (batt_soc_pct_ < 0.0f) {
        batt_soc_pct_ = 0.0f;
    }
    if (batt_soc_pct_ > 100.0f) {
        batt_soc_pct_ = 100.0f;
    }

    batt_low_ = (batt_soc_pct_ <= BATTERY_LOW_SOC_PCT);

#if BATTERY_LOW_LATCH
    if (batt_low_ && !fault_) {
        fault_ = true;
        stop("battery low SOC");
        MOWER_LOG.printf("[BATT] low SOC=%.1f%% — latched\n", (double)batt_soc_pct_);
    }
#endif
}

void Mower::updateRgb()
{
    if (!ready_) {
        return;
    }
    int32_t color = MOWER_RGB_IDLE;
    if (running_) {
        if (load_ == Load::Medium || load_ == Load::Heavy ||
            load_ == Load::StallLike) {
            color = MOWER_RGB_LOAD;
        } else {
            color = MOWER_RGB_RUN;
        }
    }
    if (fault_) {
        color = MOWER_RGB_FAULT;
    }
    if (color == last_rgb_) {
        return;
    }
    roller_.setRGB(color);
    last_rgb_ = color;
}

Mower::Status Mower::status() const
{
    Status s;
    s.ready            = ready_;
    s.running          = running_;
    s.fault            = fault_;
    s.gear             = gear_;
    s.target_raw       = target_;
    s.cmd_raw          = cmd_;
    s.speed            = speed_;
    s.current          = current_;
    s.vin              = vin_;
    s.temp             = temp_;
    s.err              = err_;
    s.sys              = sys_;
    s.load             = load_;
    s.ramping          = isRamping();
    s.soft_stall_count = soft_stall_;
    s.batt_power_w          = batt_power_w_;
    s.batt_energy_mwh       = batt_energy_mwh_;
    s.batt_soc_pct          = batt_soc_pct_;
    s.batt_used_mah         = batt_used_mah_;
    s.batt_low      = batt_low_;
    s.fault_reason  = fault_reason_;
    return s;
}

void Mower_poll()
{
    static uint32_t last_poll_ms  = 0;
    static uint32_t last_print_ms = 0;

    const uint32_t now = millis();
    if (now - last_poll_ms < (uint32_t)LOOP_PERIOD_MS) {
        return;
    }
    last_poll_ms = now;

    g_mower.update();

    if (now - last_print_ms >= (uint32_t)PRINT_PERIOD_MS) {
        last_print_ms = now;
        const Mower::Status s = g_mower.status();
        if (s.running || s.fault || s.batt_low) {
            /* ~11 fields from Status for serial debug (one line) */
            /* rpm/tgt in RPM (target_raw is x100). */
            MOWER_LOG.printf(
                "run=%d rpm=%.0f tgt=%.0f gear=%d I=%.1f V=%.2f P=%.2fW "
                "soc=%.1f%% used=%.1fmAh load=%s fault=%d ramp=%d\n",
                s.running ? 1 : 0, (double)s.speed,
                (double)s.target_raw / (double)SCALE_SPEED_DIV, s.gear,
                (double)s.current, (double)s.vin, (double)s.batt_power_w,
                (double)s.batt_soc_pct, (double)s.batt_used_mah,
                Mower::loadName(s.load), s.fault ? 1 : 0, s.ramping ? 1 : 0);
        }
    }
}

void Mower_handleSerial()
{
    while (USBSerial.available()) {
        char c = (char)USBSerial.read();
        if (c == '\r' || c == '\n') {
            continue;
        }
        switch (c) {
            case 't':
            case 'T':
            case ' ':
                g_mower.toggle();
                break;
            case '1':
                g_mower.setGear(GEAR_ECO);
                break;
            case '2':
                g_mower.setGear(GEAR_NORMAL);
                break;
            case '3':
                g_mower.setGear(GEAR_TURBO);
                break;
            case 'e':
            case 'E':
                g_mower.eStop();
                break;
            case 'r':
            case 'R':
                g_mower.clearFault();
                break;
            case 'b':
            case 'B':
                g_mower.resetEnergy();
                break;
            case 's':
            case 'S': {
                const Mower::Status s = g_mower.status();
                MOWER_LOG.printf(
                    "ready=%d run=%d SOC=%.1f%% P=%.2fW V=%.2f I=%.1f "
                    "used=%.1f fault=%d low=%d\n",
                    s.ready ? 1 : 0, s.running ? 1 : 0, s.batt_soc_pct, s.batt_power_w,
                    s.vin, s.current, s.batt_used_mah, s.fault ? 1 : 0,
                    s.batt_low ? 1 : 0);
                break;
            }
            case 'h':
            case 'H':
            case '?':
                MOWER_LOG.println("---- Mower (UI) ----");
                MOWER_LOG.println("  t 1/2/3 e r b s h");
                break;
            default:
                break;
        }
    }
}
