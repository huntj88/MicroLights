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

typedef struct {
    uint8_t modeCount;
    uint8_t minutesUntilAutoOff;
    uint8_t minutesUntilLockAfterAutoOff;
    uint8_t equationEvalIntervalMs;
    bool enableChargerSerial;
} ChipSettings;

#define DEFAULT_MODE_COUNT 0
#define DEFAULT_MINUTES_UNTIL_AUTO_OFF 90
#define DEFAULT_MINUTES_UNTIL_LOCK_AFTER_AUTO_OFF 10
#define DEFAULT_EQUATION_EVAL_INTERVAL_MS 20
#define DEFAULT_ENABLE_CHARGER_SERIAL false

static inline void chipSettingsInitDefaults(ChipSettings *settings) {
    settings->modeCount = DEFAULT_MODE_COUNT;
    settings->minutesUntilAutoOff = DEFAULT_MINUTES_UNTIL_AUTO_OFF;
    settings->minutesUntilLockAfterAutoOff = DEFAULT_MINUTES_UNTIL_LOCK_AFTER_AUTO_OFF;
    settings->equationEvalIntervalMs = DEFAULT_EQUATION_EVAL_INTERVAL_MS;
    settings->enableChargerSerial = DEFAULT_ENABLE_CHARGER_SERIAL;
}

#endif /* INC_MODEL_CHIP_SETTINGS_H_ */
