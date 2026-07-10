#ifndef MOWER_CONFIG_H
#define MOWER_CONFIG_H

// ---- Hardware: CoreS3 Lite Port A (I2C) ----
// Official: SDA = G2, SCL = G1
#define ROLLER_I2C_ADDR   0x64
#define ROLLER_I2C_SDA    2
#define ROLLER_I2C_SCL    1
#define ROLLER_I2C_HZ     400000

// ---- Power profile ----
// 0 = 5V Grove bring-up (logic verify; limited RPM)
// 1 = 6–16V PWR485 (full speed later)
#ifndef MOWER_PWR_PROFILE
#define MOWER_PWR_PROFILE  0
#endif

// ---- Speed mode: 3 gears (Eco / Normal / Turbo) ----
// setSpeed raw ≈ RPM * 100 (same scale as M5 example).
// Eco 30% | Normal 70% | Turbo 100% of SPEED_FULL.
#if MOWER_PWR_PROFILE == 0
// 5V Grove: measured ceiling ~spd 650 → raw ~65000. Keep Turbo under that.
#define SPEED_FULL          60000
#define SPEED_MAX_CURRENT   50000   // milder limit on weak supply
#else
// 12V PWR485: raise after you wire HT3.96
#define SPEED_FULL          200000
#define SPEED_MAX_CURRENT   80000
#endif

#define SPEED_PCT_ECO       30
#define SPEED_PCT_NORMAL    70
#define SPEED_PCT_TURBO     100
#define SPEED_GEAR_ECO      ((int32_t)((int64_t)SPEED_FULL * SPEED_PCT_ECO / 100))
#define SPEED_GEAR_NORMAL   ((int32_t)((int64_t)SPEED_FULL * SPEED_PCT_NORMAL / 100))
#define SPEED_GEAR_TURBO    ((int32_t)((int64_t)SPEED_FULL * SPEED_PCT_TURBO / 100))

// Gear index: 1=Eco, 2=Normal, 3=Turbo
#define GEAR_ECO            1
#define GEAR_NORMAL         2
#define GEAR_TURBO          3

// ---- Telemetry scaling (matches official motor.ino) ----
#define SCALE_SPEED_DIV   100.0f
#define SCALE_CURRENT_DIV 100.0f
#define SCALE_VIN_DIV     100.0f

// ---- Load identification (raw current readback units) ----
// 5V no-load on your log was already I≈45–50 (raw ~4500–5000).
#if MOWER_PWR_PROFILE == 0
#define LOAD_LIGHT_MAX_I    6000
#define LOAD_MEDIUM_MAX_I   10000
#define LOAD_STALL_MIN_I    9000
#else
#define LOAD_LIGHT_MAX_I    1500
#define LOAD_MEDIUM_MAX_I   4000
#define LOAD_STALL_MIN_I    3000
#endif
// If |actual_speed| << |target| while current high -> stall-like load
#define LOAD_STALL_SPEED_RATIO  0.25f   // actual < 25% of target
#define LOAD_STALL_COUNT_MAX    15      // ~15 * loop period

// ---- Loop ----
#define LOOP_PERIOD_MS    100
#define PRINT_PERIOD_MS   500

// Error codes from UnitRollerI2C docs
#define ERR_NONE          0
#define ERR_OVERVOLTAGE   1
#define ERR_JAM           2

// Sys status
#define SYS_STANDBY       0
#define SYS_RUNNING       1
#define SYS_ERROR         2

#endif  // MOWER_CONFIG_H
