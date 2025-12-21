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
#include "storage.h"
#include "mode_manager.h"
#include "model/mode_state.h"
#include "settings_manager.h"

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
    void (*writeBulbLedPin)(uint8_t state);
    uint32_t (*convertTicksToMs)(uint32_t ticks);
    void (*startLedTimers)();
    void (*stopLedTimers)();

    ModeState modeState;
    uint32_t lastPatternUpdateMs;
} ChipState;

static ChipState state = {0};

static void resetPatternState() {
	modeStateReset(&state.modeState);
	if (state.convertTicksToMs) {
		state.lastPatternUpdateMs = state.convertTicksToMs(state.chipTick);
	} else {
		state.lastPatternUpdateMs = 0;
	}
}

static void loadModeIndex(uint8_t modeIndex) {
	loadMode(state.modeManager, modeIndex);
	resetPatternState();
}

static void shutdownFake() {
	loadModeIndex(FAKE_OFF_MODE_INDEX);
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
		void (*_writeBulbLedPin)(uint8_t state),
		uint32_t (*_convertTicksToMs)(uint32_t ticks),
		void (*_startLedTimers)(),
		void (*_stopLedTimers)()
) {
	state.modeManager = _modeManager;
	state.settings = _settings;
	state.button = _button;
	state.caseLed = _caseLed;
	state.chargerIC = _chargerIC;
	state.accel = _accel;
	state.writeUsbSerial = _writeUsbSerial;
	state.writeBulbLedPin = _writeBulbLedPin;
	state.convertTicksToMs = _convertTicksToMs;
	state.startLedTimers = _startLedTimers;
	state.stopLedTimers = _stopLedTimers;

	enum ChargeState chargeState = getChargingState(state.chargerIC);

	if (chargeState == notConnected) {
		loadModeIndex(0);
	} else {
		shutdownFake();
	}
}

// If timing issues cause mode to not display right (try bulb with first color only),
// move updateMode to chipTick interrupt to confirm. Interrupt will guarantee proper timing.
// If moving fixed the bug, that indicates something is slow
static void updateMode(uint32_t ms) {
	modeStateAdvance(&state.modeState, &state.modeManager->currentMode, ms);

	ModeComponent *frontComp = NULL;
	ModeComponentState *frontState = NULL;
	ModeComponent *caseComp = NULL;
	ModeComponentState *caseState = NULL;
	bool triggered = false;

	if (state.modeManager->currentMode.has_front) {
		frontComp = &state.modeManager->currentMode.front;
		frontState = &state.modeState.front;
	}

	if (state.modeManager->currentMode.has_case_comp) {
		caseComp = &state.modeManager->currentMode.case_comp;
		caseState = &state.modeState.case_comp;
	}

	if (state.modeManager->currentMode.has_accel && state.modeManager->currentMode.accel.triggers_count > 0) {
		uint8_t triggerCount = state.modeManager->currentMode.accel.triggers_count;
		if (triggerCount > MODE_ACCEL_TRIGGER_MAX) {
			triggerCount = MODE_ACCEL_TRIGGER_MAX;
		}

		for (uint8_t i = 0; i < triggerCount; i++) {
			ModeAccelTrigger *trigger = &state.modeManager->currentMode.accel.triggers[i];
			if (isOverThreshold(state.accel, trigger->threshold)) {
				triggered = true;
				ModeAccelTriggerState *triggerState = &state.modeState.accel[i];

				if (trigger->has_front) {
					frontComp = &trigger->front;
					frontState = &triggerState->front;
				}

				if (trigger->has_case_comp) {
					caseComp = &trigger->case_comp;
					caseState = &triggerState->case_comp;
				}
			} else {
				break;
			}
		}
	}

	if (state.modeManager->currentMode.has_front || triggered) {
		if (frontComp && frontState) {
			SimpleOutput output;
			if (modeStateGetSimpleOutput(frontState, frontComp, &output)) {
				if (output.type == BULB) {
					state.writeBulbLedPin(output.data.bulb == high ? 1 : 0);
				} else {
					// TODO: RGB front component support
					state.writeBulbLedPin(0);
				}
			}
		}
	} else {
		state.writeBulbLedPin(0);
	}

	if (!isEvaluatingButtonPress(state.button)) {
		if (state.modeManager->currentMode.has_case_comp || triggered) {
			if (caseComp && caseState) {
				SimpleOutput output;
				if (modeStateGetSimpleOutput(caseState, caseComp, &output) && output.type == RGB) {
					rgbShowUserColor(state.caseLed, output.data.rgb.r, output.data.rgb.g, output.data.rgb.b);
				}
			}
		} else {
			rgbShowUserColor(state.caseLed, 0, 0, 0);
		}
	}
}

void stateTask() {
	uint32_t ms = state.convertTicksToMs(state.chipTick);

	updateMode(ms);

	enum ButtonResult buttonResult = buttonInputTask(state.button, (uint16_t)ms);
	switch (buttonResult) {
	case ignore:
		break;
	case clicked:
		rgbShowSuccess(state.caseLed);
		uint8_t newModeIndex = state.modeManager->currentModeIndex + 1;
		if (newModeIndex >= state.settings->modeCount) {
			newModeIndex = 0;
		}
		loadModeIndex(newModeIndex);
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

	rgbTask(state.caseLed, (uint16_t)ms);
	mc3479Task(state.accel, (uint16_t)ms);

	bool unplugLockEnabled = isFakeOff(state.modeManager);
	bool chargeLedEnabled =  isFakeOff(state.modeManager) && !isEvaluatingButtonPress(state.button);
	chargerTask(state.chargerIC, (uint16_t)ms, unplugLockEnabled, chargeLedEnabled);
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

		uint16_t ticksUntilAutoOff = state.settings->minutesUntilAutoOff * 60 / 10; // auto off timer running at 0.1hz
		bool autoOffTimerDone = state.ticksSinceLastUserActivity > ticksUntilAutoOff;
		if (isFakeOff(state.modeManager)) {
			uint16_t ticksUntilLockAfterAutoOff = state.settings->minutesUntilLockAfterAutoOff * 60 / 10;
			autoOffTimerDone = state.ticksSinceLastUserActivity > ticksUntilLockAfterAutoOff;
		}

		if (autoOffTimerDone) {
			if (isFakeOff(state.modeManager)) {
				lock(state.chargerIC);
			} else {
				state.ticksSinceLastUserActivity = 0; // restart timer to transition from fakeOff to shipMode
				shutdownFake();
			}
		}
	}
}
