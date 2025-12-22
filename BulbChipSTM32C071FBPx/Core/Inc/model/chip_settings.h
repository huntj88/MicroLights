/*
 * chip_settings.h
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#ifndef INC_MODEL_CHIP_SETTINGS_H_
#define INC_MODEL_CHIP_SETTINGS_H_

typedef struct {
    uint8_t modeCount;
    uint8_t minutesUntilAutoOff;
    uint8_t minutesUntilLockAfterAutoOff;
} ChipSettings;

#endif /* INC_MODEL_CHIP_SETTINGS_H_ */
