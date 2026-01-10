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

#define DEFAULT_MODE_COUNT 0
#define DEFAULT_MINUTES_UNTIL_AUTO_OFF 90
#define DEFAULT_MINUTES_UNTIL_LOCK_AFTER_AUTO_OFF 10
#define DEFAULT_EQUATION_EVAL_INTERVAL_MS 20
#define DEFAULT_ENABLE_CHARGER_SERIAL false
#define DEFAULT_ENABLE_I2C_FAILURE_REPORTING \
    false  // TODO: change to enum ALL, ERRORS, NONE, defaults json will need to contain options.

// X-Macro to define settings: X(type, name, default_value)
#define CHIP_SETTINGS_MAP(X)                                                            \
    X(uint8_t, modeCount, DEFAULT_MODE_COUNT)                                           \
    X(uint8_t, minutesUntilAutoOff, DEFAULT_MINUTES_UNTIL_AUTO_OFF)                     \
    X(uint8_t, minutesUntilLockAfterAutoOff, DEFAULT_MINUTES_UNTIL_LOCK_AFTER_AUTO_OFF) \
    X(uint8_t, equationEvalIntervalMs, DEFAULT_EQUATION_EVAL_INTERVAL_MS)               \
    X(bool, enableChargerSerial, DEFAULT_ENABLE_CHARGER_SERIAL)                         \
    X(bool, enableI2cFailureReporting, DEFAULT_ENABLE_I2C_FAILURE_REPORTING)

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
