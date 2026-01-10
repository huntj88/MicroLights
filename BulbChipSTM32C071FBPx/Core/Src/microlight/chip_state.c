/*
 * chip_state.c
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "microlight/chip_state.h"
#include "microlight/json/command_parser.h"
#include "microlight/json/mode_parser.h"
#include "microlight/mode_manager.h"
#include "microlight/model/mode_state.h"
#include "microlight/settings_manager.h"

typedef struct {
    volatile uint32_t chipTick;
    volatile uint32_t ticksSinceLastUserActivity;
    volatile bool autoOffTimerTriggered;

    ModeManager *modeManager;
    ChipSettings *settings;

    // Devices
    Button *button;
    RGBLed *caseLed;
    BQ25180 *chargerIC;
    MC3479 *accel;

    // Callbacks
    Log log;
    uint32_t (*convertTicksToMilliseconds)(uint32_t ticks);
} ChipState;

// TODO: move this state to microlight
static ChipState state = {0};

void configureChipState(
    ModeManager *modeManager,
    ChipSettings *settings,
    Button *button,
    BQ25180 *chargerIC,
    MC3479 *accel,
    RGBLed *caseLed,
    Log log,
    uint32_t (*convertTicksToMilliseconds)(uint32_t ticks)) {
    state.modeManager = modeManager;
    state.settings = settings;
    state.button = button;
    state.caseLed = caseLed;
    state.chargerIC = chargerIC;
    state.accel = accel;
    state.log = log;
    state.convertTicksToMilliseconds = convertTicksToMilliseconds;
    enum ChargeState chargeState = getChargingState(state.chargerIC);

    if (chargeState == notConnected) {
        loadMode(state.modeManager, 0);
    } else {
        // Enter fake off mode when charging, show led status by enabling led timers
        fakeOffMode(state.modeManager, true);
    }
}

static void handleAutoOffTimer() {
    if (state.autoOffTimerTriggered) {
        state.autoOffTimerTriggered = false;

        if (getChargingState(state.chargerIC) == notConnected) {
            state.ticksSinceLastUserActivity++;

            uint16_t ticksUntilAutoOff =
                state.settings->minutesUntilAutoOff * 60 / 10;  // auto off timer running at 0.1hz
            bool autoOffTimerDone = state.ticksSinceLastUserActivity > ticksUntilAutoOff;
            if (isFakeOff(state.modeManager)) {
                uint16_t ticksUntilLockAfterAutoOff =
                    state.settings->minutesUntilLockAfterAutoOff * 60 / 10;
                autoOffTimerDone = state.ticksSinceLastUserActivity > ticksUntilLockAfterAutoOff;
            }

            if (autoOffTimerDone) {
                if (isFakeOff(state.modeManager)) {
                    lock(state.chargerIC);
                } else {
                    // restart timer for transition from fakeOff to shipMode
                    state.ticksSinceLastUserActivity = 0;

                    // enter fake off mode
                    bool enableLedTimers = getChargingState(state.chargerIC) != notConnected;
                    fakeOffMode(state.modeManager, enableLedTimers);
                }
            }
        }
    }
}

void stateTask() {
    handleAutoOffTimer();
    uint32_t milliseconds = state.convertTicksToMilliseconds(state.chipTick);

    bool canUpdateCaseLed = !isEvaluatingButtonPress(state.button);
    modeTask(
        state.modeManager, milliseconds, canUpdateCaseLed, state.settings->equationEvalIntervalMs);

    enum ButtonResult buttonResult = buttonInputTask(state.button, milliseconds);
    switch (buttonResult) {
        case ignore:
            break;
        case clicked:
            rgbShowSuccess(state.caseLed);
            uint8_t newModeIndex = state.modeManager->currentModeIndex + 1;
            if (newModeIndex >= state.settings->modeCount) {
                newModeIndex = 0;
            }
            loadMode(state.modeManager, newModeIndex);
            const char *blah = "clicked\n";
            state.log(blah, strlen(blah));
            break;
        case shutdown:
            bool enableLedTimers = getChargingState(state.chargerIC) != notConnected;
            fakeOffMode(state.modeManager, enableLedTimers);
            break;
        case lockOrHardwareReset:
            lock(state.chargerIC);
            break;
    }

    if (buttonResult != ignore) {
        state.ticksSinceLastUserActivity = 0;
    }

    rgbTask(state.caseLed, milliseconds);
    mc3479Task(state.accel, milliseconds);

    bool unplugLockEnabled = isFakeOff(state.modeManager);
    bool chargeLedEnabled = isFakeOff(state.modeManager) && canUpdateCaseLed;
    chargerTask(
        state.chargerIC,
        milliseconds,
        unplugLockEnabled,
        chargeLedEnabled,
        state.settings->enableChargerSerial);
}

// TODO: Rate of chipTick interrupt should be configurable
void chipTickInterrupt() {
    state.chipTick++;
}

// Auto off timer running at 0.1 hz
// 12 megahertz / 65535 / 1831 = 0.1 hz
void autoOffTimerInterrupt() {
    state.autoOffTimerTriggered = true;
}
