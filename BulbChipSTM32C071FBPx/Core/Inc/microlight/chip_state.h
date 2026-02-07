/*
 * chip_state.h
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#ifndef INC_CHIP_STATE_H_
#define INC_CHIP_STATE_H_

#include <stdbool.h>
#include <stdint.h>

#include "microlight/device/bq25180.h"
#include "microlight/device/button.h"
#include "microlight/device/mc3479.h"
#include "microlight/device/rgb_led.h"
#include "microlight/json/command_parser.h"
#include "microlight/mode_manager.h"
#include "microlight/model/log.h"
#include "microlight/settings_manager.h"

typedef struct {
    ModeManager *modeManager;
    ChipSettings *settings;

    // Devices
    Button *button;
    RGBLed *caseLed;
    BQ25180 *chargerIC;
    MC3479 *accel;

    // Callbacks
    void (*enableChipTickTimer)(bool enable);
    void (*enableCaseLedTimer)(bool enable);
    void (*enableFrontLedTimer)(bool enable);
    Log log;

    // State
    uint32_t ticksSinceLastUserActivity;  // auto off timer ticks at 0.1 hz
} ChipState;

bool configureChipState(
    ChipState *state,
    ModeManager *modeManager,
    ChipSettings *settings,
    Button *button,
    BQ25180 *chargerIC,
    MC3479 *accel,
    RGBLed *caseLed,
    void (*enableChipTickTimer)(bool enable),
    void (*enableCaseLedTimer)(bool enable),
    void (*enableFrontLedTimer)(bool enable),
    Log log);

typedef struct StateTaskFlags {
    bool autoOffTimerInterruptTriggered;
    bool buttonInterruptTriggered;
    bool chargerInterruptTriggered;
} StateTaskFlags;

void stateTask(ChipState *state, uint32_t milliseconds, StateTaskFlags flags);

#endif /* INC_CHIP_STATE_H_ */
