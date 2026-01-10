/*
 * chip_state.h
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#ifndef INC_CHIP_STATE_H_
#define INC_CHIP_STATE_H_

#include <stdint.h>

#include "microlight/device/bq25180.h"
#include "microlight/device/button.h"
#include "microlight/device/mc3479.h"
#include "microlight/device/rgb_led.h"
#include "microlight/json/command_parser.h"
#include "microlight/mode_manager.h"
#include "microlight/model/log.h"
#include "microlight/settings_manager.h"

void configureChipState(
    ModeManager *modeManager,
    ChipSettings *settings,
    Button *button,
    BQ25180 *chargerIC,
    MC3479 *accel,
    RGBLed *caseLed,
    Log log,
    uint32_t (*convertTicksToMilliseconds)(uint32_t ticks));

void stateTask(
    bool chipTickTriggered,
    bool autoOffTimerTriggered,
    bool buttonInterruptTriggered,
    bool chargerInterruptTriggered);

#endif /* INC_CHIP_STATE_H_ */
