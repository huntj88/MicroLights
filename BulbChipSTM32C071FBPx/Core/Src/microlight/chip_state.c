/*
 * chip_state.c
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "microlight/chip_state.h"
#include "microlight/json/command_parser.h"
#include "microlight/json/mode_parser.h"
#include "microlight/mode_manager.h"
#include "microlight/model/mode_state.h"
#include "microlight/settings_manager.h"

static void enterShutdown(ChipState *state, enum ChargeState chargeState);
static void prepareForLowPowerShutdown(ChipState *state);
static void syncLedWhiteBalance(ChipState *state);

bool configureChipState(ChipState *state, ChipDependencies deps) {
    if (!state || !deps.modeManager || !deps.settings || !deps.button || !deps.chargerIC ||
        !deps.accel || !deps.caseLed || !deps.frontLed || !deps.enableChipTickTimer ||
        !deps.enableCaseLedTimer || !deps.enableFrontLedTimer || !deps.enableAutoOffTimer ||
        !deps.enableUsbClock || !deps.enterStandbyMode || !deps.waitForButtonWakeOrAutoLock ||
        !deps.systemReset || !deps.log) {
        return false;
    }

    state->deps = deps;
    state->ticksSinceLastUserActivity = 0;
    state->lastChipTickEnabled = false;
    state->lastCasePwmEnabled = false;
    state->lastFrontPwmEnabled = false;
    syncLedWhiteBalance(state);
    enum ChargeState initialChargeState = getChargingState(state->deps.chargerIC, 0);
    bool usbNeeded = initialChargeState != notConnected;
    state->lastUsbClockEnabled = usbNeeded;
    state->deps.enableUsbClock(usbNeeded);

    if (initialChargeState == notConnected) {
        loadMode(state->deps.modeManager, 0);
    } else {
        // Enter fake off mode when charging, show led status by enabling charge led timers
        fakeOffMode(state->deps.modeManager);
    }
    return true;
}

static void syncLedWhiteBalance(ChipState *state) {
    if (!state || !state->deps.settings) {
        return;
    }

    rgbSetWhiteBalance(
        state->deps.caseLed,
        (RGBWhiteBalance){
            .red = state->deps.settings->caseWhiteBalanceRed,
            .green = state->deps.settings->caseWhiteBalanceGreen,
            .blue = state->deps.settings->caseWhiteBalanceBlue,
        });
    rgbSetWhiteBalance(
        state->deps.frontLed,
        (RGBWhiteBalance){
            .red = state->deps.settings->frontWhiteBalanceRed,
            .green = state->deps.settings->frontWhiteBalanceGreen,
            .blue = state->deps.settings->frontWhiteBalanceBlue,
        });
}

// Auto off timer running at 0.1 hz
// 12 megahertz / 1875 / 64000 = 0.1 hz (TIM17: prescaler=1874, period=63999)
static bool handleAutoOffTimer(
    ChipState *state, bool timerTriggered, enum ChargeState chargeState) {
    if (!timerTriggered || chargeState != notConnected ||
        state->deps.settings->shutdownPolicy == manualShutdownOnly ||
        isFakeOff(state->deps.modeManager)) {
        return false;
    }

    state->ticksSinceLastUserActivity++;

    uint16_t ticksUntilAutoOff =
        state->deps.settings->minutesUntilAutoOff * 6;  // auto off timer running at 0.1hz
    bool autoOffTimerDone = state->ticksSinceLastUserActivity > ticksUntilAutoOff;
    if (autoOffTimerDone) {
        state->ticksSinceLastUserActivity = 0;
        enterShutdown(state, chargeState);
        return true;
    }

    return false;
}

static void enterShutdown(ChipState *state, enum ChargeState chargeState) {
    if (chargeState != notConnected) {
        fakeOffMode(state->deps.modeManager);
        return;
    }

    prepareForLowPowerShutdown(state);

    if (state->deps.settings->shutdownPolicy == autoOffAndAutoLock) {
        uint16_t lockThresholdMinutes = state->deps.settings->minutesUntilLockAfterAutoOff;
        if (lockThresholdMinutes == 0U) {
            lock(state->deps.chargerIC);
            return;
        }

        bool wokeFromButton = state->deps.waitForButtonWakeOrAutoLock(lockThresholdMinutes);
        if (wokeFromButton) {
            state->deps.systemReset();
            return;
        }

        lock(state->deps.chargerIC);
        return;
    }

    state->deps.enterStandbyMode();
}

static void prepareForLowPowerShutdown(ChipState *state) {
    state->deps.enableAutoOffTimer(false);

    // The BQ25180 host watchdog must be disabled before long MCU sleep intervals,
    // or it can reset charger state while we are intentionally in Stop/Standby.
    // Will be re initialized when the system wakes up
    disableWatchdog(state->deps.chargerIC);
    mc3479Disable(state->deps.accel);

    if (state->lastChipTickEnabled) {
        state->deps.enableChipTickTimer(false);
        state->lastChipTickEnabled = false;
    }

    if (state->lastCasePwmEnabled) {
        state->deps.enableCaseLedTimer(false);
        state->lastCasePwmEnabled = false;
    }

    // Force GPIO mode and drive the legacy bulb/front-blue pin low even if
    // cached PWM state says the front timer was already disabled.
    state->deps.enableFrontLedTimer(false);
    state->lastFrontPwmEnabled = false;

    if (state->lastUsbClockEnabled) {
        state->deps.enableUsbClock(false);
        state->lastUsbClockEnabled = false;
    }
}

static void applyTimerPolicy(
    ChipState *state,
    ModeOutputs outputs,
    bool evaluatingButtonPress,
    enum ChargeState chargeState) {
    if (!state || !state->deps.modeManager) {
        return;
    }

    ModeManager *manager = state->deps.modeManager;
    bool fakeOff = isFakeOff(manager);
    bool chargeLedEnabled = fakeOff && chargeState != notConnected;
    bool frontRgbActive = outputs.frontValid && outputs.frontType == RGB;
    bool caseRgbActive = outputs.caseValid;

    bool chipTickEnabled = !fakeOff || chargeLedEnabled || evaluatingButtonPress;
    bool casePwmEnabled = chargeLedEnabled || caseRgbActive || evaluatingButtonPress;
    bool frontPwmEnabled = frontRgbActive;
    bool usbClockEnabled = chargeState != notConnected;

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
    if (usbClockEnabled != state->lastUsbClockEnabled) {
        state->deps.enableUsbClock(usbClockEnabled);
        state->lastUsbClockEnabled = usbClockEnabled;
    }
}

void stateTask(ChipState *state, uint32_t milliseconds, StateTaskFlags flags) {
    syncLedWhiteBalance(state);

    enum ChargeState chargeState = getChargingState(state->deps.chargerIC, milliseconds);
    if (handleAutoOffTimer(state, flags.autoOffTimerInterruptTriggered, chargeState)) {
        return;
    }

    bool caseLedReservedForButton = isEvaluatingButtonPress(state->deps.button);
    bool caseLedReservedForStatus = isFakeOff(state->deps.modeManager);
    bool allowModeCaseLedUpdates = !caseLedReservedForButton && !caseLedReservedForStatus;

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
            enterShutdown(state, chargeState);
            if (chargeState == notConnected) {
                return;
            }
            // Clear outputs so applyTimerPolicy disables front/case PWM timers
            outputs.frontValid = false;
            outputs.caseValid = false;
            break;
        }
        case lockOrHardwareReset:
            lock(state->deps.chargerIC);
            enterShutdown(state, chargeState);
            break;
    }

    if (buttonResult != ignore) {
        state->ticksSinceLastUserActivity = 0;
    }

    bool evaluatingButtonPress = isEvaluatingButtonPress(state->deps.button);
    applyTimerPolicy(
        state, outputs, evaluatingButtonPress || flags.buttonInterruptTriggered, chargeState);

    // No transient status show on front LED, only call for case LED.
    rgbTransientTask(state->deps.caseLed, milliseconds);
    mc3479Task(state->deps.accel, milliseconds);
    chargerTask(
        state->deps.chargerIC,
        milliseconds,
        (ChargerTaskFlags){
            .interruptTriggered = flags.chargerInterruptTriggered,
            .unplugLockEnabled = isFakeOff(state->deps.modeManager),
            .chargeLedEnabled = isFakeOff(state->deps.modeManager) && chargeState != notConnected &&
                                !evaluatingButtonPress,
            .serialEnabled = state->deps.settings->enableChargerSerial});
}
