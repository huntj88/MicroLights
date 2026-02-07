/*
 * chip_state.c
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "microlight/chip_state.h"
#include "microlight/json/command_parser.h"
#include "microlight/json/mode_parser.h"
#include "microlight/mode_manager.h"
#include "microlight/model/mode_state.h"
#include "microlight/settings_manager.h"

bool configureChipState(ChipState *state, ChipDependencies deps) {
    if (!state || !deps.modeManager || !deps.settings || !deps.button || !deps.chargerIC ||
        !deps.accel || !deps.caseLed || !deps.enableChipTickTimer || !deps.enableCaseLedTimer ||
        !deps.enableFrontLedTimer || !deps.log) {
        return false;
    }
    state->deps = deps;
    state->ticksSinceLastUserActivity = 0;
    state->lastChipTickEnabled = false;
    state->lastCasePwmEnabled = false;
    state->lastFrontPwmEnabled = false;
    enum ChargeState chargeState = getChargingState(state->deps.chargerIC);

    if (chargeState == notConnected) {
        loadMode(state->deps.modeManager, 0);
    } else {
        // Enter fake off mode when charging, show led status by enabling charge led timers
        fakeOffMode(state->deps.modeManager);
    }
    return true;
}

// Auto off timer running at 0.1 hz
// 12 megahertz / 1875 / 64000 = 0.1 hz (TIM17: prescaler=1874, period=63999)
static void handleAutoOffTimer(ChipState *state, bool timerTriggered) {
    if (timerTriggered) {
        if (getChargingState(state->deps.chargerIC) == notConnected) {
            state->ticksSinceLastUserActivity++;

            uint16_t ticksUntilAutoOff =
                state->deps.settings->minutesUntilAutoOff * 6;  // auto off timer running at 0.1hz
            bool autoOffTimerDone = state->ticksSinceLastUserActivity > ticksUntilAutoOff;
            if (isFakeOff(state->deps.modeManager)) {
                uint16_t ticksUntilLockAfterAutoOff =
                    state->deps.settings->minutesUntilLockAfterAutoOff * 6;
                autoOffTimerDone = state->ticksSinceLastUserActivity > ticksUntilLockAfterAutoOff;
            }

            if (autoOffTimerDone) {
                if (isFakeOff(state->deps.modeManager)) {
                    lock(state->deps.chargerIC);
                } else {
                    // restart timer for transition from fakeOff to shipMode
                    state->ticksSinceLastUserActivity = 0;

                    // enter fake off mode
                    fakeOffMode(state->deps.modeManager);
                }
            }
        }
    }
}

static void applyTimerPolicy(ChipState *state, ModeOutputs outputs, bool evaluatingButtonPress) {
    if (!state || !state->deps.modeManager) {
        return;
    }

    ModeManager *manager = state->deps.modeManager;
    bool fakeOff = isFakeOff(manager);
    bool chargeLedEnabled = fakeOff && getChargingState(state->deps.chargerIC) != notConnected;
    bool frontRgbActive = outputs.frontValid && outputs.frontType == RGB;
    bool caseRgbActive = outputs.caseValid;

    bool chipTickEnabled = !fakeOff || chargeLedEnabled || evaluatingButtonPress;
    bool casePwmEnabled = chargeLedEnabled || caseRgbActive || evaluatingButtonPress;
    bool frontPwmEnabled = frontRgbActive;

    if (chipTickEnabled != state->lastChipTickEnabled) {
        state->deps.enableChipTickTimer(chipTickEnabled);
        state->lastChipTickEnabled = chipTickEnabled;
    }
    if (casePwmEnabled != state->lastCasePwmEnabled) {
        state->deps.enableCaseLedTimer(casePwmEnabled);
        state->lastCasePwmEnabled = casePwmEnabled;
    }
    if (frontPwmEnabled != state->lastFrontPwmEnabled) {
        state->deps.enableFrontLedTimer(frontPwmEnabled);
        state->lastFrontPwmEnabled = frontPwmEnabled;
    }
}

void stateTask(ChipState *state, uint32_t milliseconds, StateTaskFlags flags) {
    handleAutoOffTimer(state, flags.autoOffTimerInterruptTriggered);

    bool caseLedBusyWithButton = isEvaluatingButtonPress(state->deps.button);
    bool caseLedBusyWithCharge = isFakeOff(state->deps.modeManager);
    bool allowModeCaseLedUpdates = !caseLedBusyWithButton && !caseLedBusyWithCharge;

    ModeOutputs outputs = modeTask(
        state->deps.modeManager,
        milliseconds,
        allowModeCaseLedUpdates,
        state->deps.settings->equationEvalIntervalMs);

    enum ButtonResult buttonResult =
        buttonInputTask(state->deps.button, milliseconds, flags.buttonInterruptTriggered);
    switch (buttonResult) {
        case ignore:
            break;
        case clicked: {
            const char *clicked = "clicked\n";
            state->deps.log(clicked, strlen(clicked));
            rgbShowSuccess(state->deps.caseLed);
            uint8_t newModeIndex = state->deps.modeManager->currentModeIndex + 1;
            if (newModeIndex >= state->deps.settings->modeCount) {
                newModeIndex = 0;
            }
            loadMode(state->deps.modeManager, newModeIndex);
            char msg[48];
            int len = snprintf(
                msg, sizeof(msg), "{\"event\":\"modeChanged\",\"index\":%u}\n", newModeIndex);
            if (len > 0) {
                state->deps.log(msg, (size_t)len);
            }
            break;
        }
        case shutdown: {
            fakeOffMode(state->deps.modeManager);
            // Clear outputs so applyTimerPolicy disables front/case PWM timers
            outputs.frontValid = false;
            outputs.caseValid = false;
            break;
        }
        case lockOrHardwareReset:
            lock(state->deps.chargerIC);
            break;
    }

    if (buttonResult != ignore) {
        state->ticksSinceLastUserActivity = 0;
    }

    bool evaluatingButtonPress = isEvaluatingButtonPress(state->deps.button);
    applyTimerPolicy(state, outputs, evaluatingButtonPress || flags.buttonInterruptTriggered);

    rgbTask(state->deps.caseLed, milliseconds);
    mc3479Task(state->deps.accel, milliseconds);
    chargerTask(
        state->deps.chargerIC,
        milliseconds,
        (ChargerTaskFlags){
            .interruptTriggered = flags.chargerInterruptTriggered,
            .unplugLockEnabled = isFakeOff(state->deps.modeManager),
            .chargeLedEnabled = isFakeOff(state->deps.modeManager) &&
                                getChargingState(state->deps.chargerIC) != notConnected &&
                                !evaluatingButtonPress,
            .serialEnabled = state->deps.settings->enableChargerSerial});
}
