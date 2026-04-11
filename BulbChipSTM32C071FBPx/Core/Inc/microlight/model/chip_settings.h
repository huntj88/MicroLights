/*
 * chip_settings.h
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#ifndef INC_MODEL_CHIP_SETTINGS_H_
#define INC_MODEL_CHIP_SETTINGS_H_

#include <stdbool.h>
#include <stdint.h>

#define SHUTDOWN_POLICY_MAP(X)                            \
    X(manualShutdownOnly, 0, "Manual shutdown only")      \
    X(autoOffNoAutoLock, 1, "Auto off without auto lock") \
    X(autoOffAndAutoLock, 2, "Auto off and auto lock")

enum ShutdownPolicy {
#define X_ENUM(name, value, label) name = value,
    SHUTDOWN_POLICY_MAP(X_ENUM)
#undef X_ENUM
};

#define DEFAULT_MODE_COUNT 0
#define DEFAULT_MINUTES_UNTIL_AUTO_OFF 90
#define DEFAULT_MINUTES_UNTIL_LOCK_AFTER_AUTO_OFF 10
#define DEFAULT_EQUATION_EVAL_INTERVAL_MS 20
#define DEFAULT_SHUTDOWN_POLICY autoOffAndAutoLock
#define DEFAULT_ENABLE_CHARGER_SERIAL false
#define DEFAULT_ENABLE_I2C_FAILURE_REPORTING \
    false  // TODO: change to enum ALL, ERRORS, NONE, defaults json will need to contain options.
#define DEFAULT_FRONT_WHITE_BALANCE_RED 255
#define DEFAULT_FRONT_WHITE_BALANCE_GREEN 110
#define DEFAULT_FRONT_WHITE_BALANCE_BLUE 60
#define DEFAULT_CASE_WHITE_BALANCE_RED 140
#define DEFAULT_CASE_WHITE_BALANCE_GREEN 210
#define DEFAULT_CASE_WHITE_BALANCE_BLUE 255

#ifdef MICROLIGHT_LEGACY_PCB_BUTTON_PA7
_Static_assert(
    DEFAULT_SHUTDOWN_POLICY == autoOffAndAutoLock,
    "Legacy PA7 button build requires default shutdownPolicy = 2 (autoOffAndAutoLock)");
#endif

// X-Macro to define settings: X(type, name, default_value)
#define CHIP_SETTINGS_MAP(X)                                                            \
    X(uint8_t, modeCount, DEFAULT_MODE_COUNT)                                           \
    X(uint8_t, minutesUntilAutoOff, DEFAULT_MINUTES_UNTIL_AUTO_OFF)                     \
    X(uint8_t, minutesUntilLockAfterAutoOff, DEFAULT_MINUTES_UNTIL_LOCK_AFTER_AUTO_OFF) \
    X(uint8_t, equationEvalIntervalMs, DEFAULT_EQUATION_EVAL_INTERVAL_MS)               \
    X(uint8_t, shutdownPolicy, DEFAULT_SHUTDOWN_POLICY)                                 \
    X(bool, enableChargerSerial, DEFAULT_ENABLE_CHARGER_SERIAL)                         \
    X(bool, enableI2cFailureReporting, DEFAULT_ENABLE_I2C_FAILURE_REPORTING)            \
    X(uint8_t, frontWhiteBalanceRed, DEFAULT_FRONT_WHITE_BALANCE_RED)                   \
    X(uint8_t, frontWhiteBalanceGreen, DEFAULT_FRONT_WHITE_BALANCE_GREEN)               \
    X(uint8_t, frontWhiteBalanceBlue, DEFAULT_FRONT_WHITE_BALANCE_BLUE)                 \
    X(uint8_t, caseWhiteBalanceRed, DEFAULT_CASE_WHITE_BALANCE_RED)                     \
    X(uint8_t, caseWhiteBalanceGreen, DEFAULT_CASE_WHITE_BALANCE_GREEN)                 \
    X(uint8_t, caseWhiteBalanceBlue, DEFAULT_CASE_WHITE_BALANCE_BLUE)

typedef struct {
#define X_FIELDS(type, name, def) type name;
    CHIP_SETTINGS_MAP(X_FIELDS)
#undef X_FIELDS
} ChipSettings;

static inline void chipSettingsInitDefaults(ChipSettings *settings) {
#define X_DEFAULTS(type, name, def) settings->name = def;
    CHIP_SETTINGS_MAP(X_DEFAULTS)
#undef X_DEFAULTS
}

#endif /* INC_MODEL_CHIP_SETTINGS_H_ */
