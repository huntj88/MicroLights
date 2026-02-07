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

bool configureChipState(
    ChipState *state,
    ModeManager *modeManager,
    ChipSettings *settings,
    Button *button,
    BQ25180 *chargerIC,
    MC3479 *accel,
    RGBLed *caseLed,
    Log log) {
    if (!state || !modeManager || !settings || !button || !chargerIC || !accel || !caseLed ||
        !log) {
        return false;
    }
    state->modeManager = modeManager;
    state->settings = settings;
    state->button = button;
    state->caseLed = caseLed;
    state->chargerIC = chargerIC;
    state->accel = accel;
    state->log = log;
    state->ticksSinceLastUserActivity = 0;
    enum ChargeState chargeState = getChargingState(state->chargerIC);

    if (chargeState == notConnected) {
        loadMode(state->modeManager, 0);
    } else {
        // Enter fake off mode when charging, show led status by enabling charge led timers
        fakeOffMode(state->modeManager, true);
    }
    return true;
}

// Auto off timer running at 0.1 hz
// 12 megahertz / 65535 / 1831 = 0.1 hz
static void handleAutoOffTimer(ChipState *state, bool timerTriggered) {
    if (timerTriggered) {
        if (getChargingState(state->chargerIC) == notConnected) {
            state->ticksSinceLastUserActivity++;

            uint16_t ticksUntilAutoOff =
                state->settings->minutesUntilAutoOff * 6;  // auto off timer running at 0.1hz
            bool autoOffTimerDone = state->ticksSinceLastUserActivity > ticksUntilAutoOff;
            if (isFakeOff(state->modeManager)) {
                uint16_t ticksUntilLockAfterAutoOff =
                    state->settings->minutesUntilLockAfterAutoOff * 6;
                autoOffTimerDone = state->ticksSinceLastUserActivity > ticksUntilLockAfterAutoOff;
            }

            if (autoOffTimerDone) {
                if (isFakeOff(state->modeManager)) {
                    lock(state->chargerIC);
                } else {
                    // restart timer for transition from fakeOff to shipMode
                    state->ticksSinceLastUserActivity = 0;

                    // enter fake off mode
                    bool enableChargeLedTimers = getChargingState(state->chargerIC) != notConnected;
                    fakeOffMode(state->modeManager, enableChargeLedTimers);
                }
            }
        }
    }
}

void stateTask(ChipState *state, uint32_t milliseconds, StateTaskFlags flags) {
    handleAutoOffTimer(state, flags.autoOffTimerInterruptTriggered);

    // TODO: clean this up?
    bool canUpdateCaseLed = !isEvaluatingButtonPress(state->button);
    bool fakeOff = isFakeOff(state->modeManager);
    bool chargeLedEnabled = fakeOff && canUpdateCaseLed;
    bool allowModeCaseLedUpdates = canUpdateCaseLed && !chargeLedEnabled;
    
    modeTask(
        state->modeManager,
        milliseconds,
        allowModeCaseLedUpdates,
        state->settings->equationEvalIntervalMs);

    enum ButtonResult buttonResult =
        buttonInputTask(state->button, milliseconds, flags.buttonInterruptTriggered);
    switch (buttonResult) {
        case ignore:
            break;
        case clicked:
            rgbShowSuccess(state->caseLed);
            uint8_t newModeIndex = state->modeManager->currentModeIndex + 1;
            if (newModeIndex >= state->settings->modeCount) {
                newModeIndex = 0;
            }
            loadMode(state->modeManager, newModeIndex);
            const char *blah = "clicked\n";
            state->log(blah, strlen(blah));
            break;
        case shutdown:
            bool enableChargeLedTimers = getChargingState(state->chargerIC) != notConnected;
            fakeOffMode(state->modeManager, enableChargeLedTimers);
            break;
        case lockOrHardwareReset:
            lock(state->chargerIC);
            break;
    }

    if (buttonResult != ignore) {
        state->ticksSinceLastUserActivity = 0;
    }

    rgbTask(state->caseLed, milliseconds);
    mc3479Task(state->accel, milliseconds);

    // TODO: clean this up?
    fakeOff = isFakeOff(state->modeManager);
    chargeLedEnabled = fakeOff && canUpdateCaseLed;

    bool unplugLockEnabled = fakeOff;
    chargerTask(
        state->chargerIC,
        milliseconds,
        (ChargerTaskFlags){
            .interruptTriggered = flags.chargerInterruptTriggered,
            .unplugLockEnabled = unplugLockEnabled,
            .chargeLedEnabled = chargeLedEnabled,
            .serialEnabled = state->settings->enableChargerSerial});
}
