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

#include "chip_state.h"
#include "json/command_parser.h"
#include "json/mode_parser.h"
#include "mode_manager.h"
#include "model/mode_state.h"
#include "settings_manager.h"
#include "storage.h"

typedef struct {
    volatile uint32_t chipTick;
    volatile uint32_t ticksSinceLastUserActivity;

    ModeManager *modeManager;
    ChipSettings *settings;

    // Devices
    Button *button;
    RGBLed *caseLed;
    BQ25180 *chargerIC;
    MC3479 *accel;

    // Callbacks
    WriteToUsbSerial *writeUsbSerial;
    uint32_t (*convertTicksToMs)(uint32_t ticks);
    void (*startLedTimers)();
    void (*stopLedTimers)();
} ChipState;

static ChipState state = {0};

static void shutdownFake() {
    loadMode(state.modeManager, FAKE_OFF_MODE_INDEX);
    if (getChargingState(state.chargerIC) != notConnected) {
        state.startLedTimers();
    }
}

void configureChipState(
    ModeManager *_modeManager,
    ChipSettings *_settings,
    Button *_button,
    BQ25180 *_chargerIC,
    MC3479 *_accel,
    RGBLed *_caseLed,
    WriteToUsbSerial *_writeUsbSerial,
    uint32_t (*_convertTicksToMs)(uint32_t ticks),
    void (*_startLedTimers)(),
    void (*_stopLedTimers)()) {
    state.modeManager = _modeManager;
    state.settings = _settings;
    state.button = _button;
    state.caseLed = _caseLed;
    state.chargerIC = _chargerIC;
    state.accel = _accel;
    state.writeUsbSerial = _writeUsbSerial;
    state.convertTicksToMs = _convertTicksToMs;
    state.startLedTimers = _startLedTimers;
    state.stopLedTimers = _stopLedTimers;

    enum ChargeState chargeState = getChargingState(state.chargerIC);

    if (chargeState == notConnected) {
        loadMode(state.modeManager, 0);
    } else {
        shutdownFake();
    }
}

void stateTask() {
    uint32_t ms = state.convertTicksToMs(state.chipTick);

    modeTask(state.modeManager, ms, !isEvaluatingButtonPress(state.button));

    enum ButtonResult buttonResult = buttonInputTask(state.button, ms);
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
            state.writeUsbSerial(0, blah, strlen(blah));
            break;
        case shutdown:
            shutdownFake();
            break;
        case lockOrHardwareReset:
            lock(state.chargerIC);
            break;
    }

    if (buttonResult != ignore) {
        state.ticksSinceLastUserActivity = 0;
    }

    rgbTask(state.caseLed, ms);
    mc3479Task(state.accel, ms);

    bool unplugLockEnabled = isFakeOff(state.modeManager);
    bool chargeLedEnabled = isFakeOff(state.modeManager) && !isEvaluatingButtonPress(state.button);
    chargerTask(state.chargerIC, ms, unplugLockEnabled, chargeLedEnabled);
}

// TODO: Rate of chipTick interrupt should be configurable
void chipTickInterrupt() {
    state.chipTick++;
}

// Auto off timer running at 0.1 hz
// 12 megahertz / 65535 / 1831 = 0.1 hz
void autoOffTimerInterrupt() {
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
                state.ticksSinceLastUserActivity =
                    0;  // restart timer to transition from fakeOff to shipMode
                shutdownFake();
            }
        }
    }
}
