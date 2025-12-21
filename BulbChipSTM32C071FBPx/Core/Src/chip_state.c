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
#include "settings_manager.h"

typedef struct {
    volatile uint16_t chipTick;
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
    float (*getMillisecondsPerChipTick)();
    void (*startLedTimers)();
    void (*stopLedTimers)();
} ChipState;

static ChipState state = {0};

static void loadModeIndex(uint8_t modeIndex) {
	loadMode(state.modeManager, modeIndex);
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
		float (*_getMillisecondsPerChipTick)(),
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
	state.getMillisecondsPerChipTick = _getMillisecondsPerChipTick;
	state.startLedTimers = _startLedTimers;
	state.stopLedTimers = _stopLedTimers;

	enum ChargeState chargeState = getChargingState(state.chargerIC);

	if (chargeState == notConnected) {
		loadModeIndex(0);
	} else {
		shutdownFake();
	}
}

void stateTask() {
	float millisPerTick = state.getMillisecondsPerChipTick();
	uint32_t ms = (uint32_t)(state.chipTick * millisPerTick);

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

// Helper to get output from a simple pattern at a specific time
// TODO: pass in output pointer instead of returning by value, return bool for success/failure, return false is changeAt_count is 0
// TODO: extract to mode util file, for testing
static SimpleOutput getOutputFromSimplePattern(SimplePattern *pattern, uint32_t ms) {
	uint32_t patternTime = ms % pattern->duration;
	// Find the last change that occurred before or at patternTime
	uint8_t lastChangeIndex = 0;
	for (uint8_t i = 0; i < pattern->changeAt_count; i++) {
		if (pattern->changeAt[i].ms <= patternTime) {
			lastChangeIndex = i;
		} else {
			break;
		}
	}

	return pattern->changeAt[lastChangeIndex].output;
}

static void updateMode(uint32_t ms) {
	// Check triggers
	ModeComponent frontComp = state.modeManager->currentMode.front;
	ModeComponent caseComp = state.modeManager->currentMode.case_comp;
	bool triggered = false;

	if (state.modeManager->currentMode.has_accel && state.modeManager->currentMode.accel.triggers_count > 0) {
		// Triggers are sorted by threshold ascending. Find the highest threshold that is met.
		for (uint8_t i = 0; i < state.modeManager->currentMode.accel.triggers_count; i++) {
			if (isOverThreshold(state.accel, state.modeManager->currentMode.accel.triggers[i].threshold)) {
				ModeAccelTrigger trigger = state.modeManager->currentMode.accel.triggers[i];

				// TODO: indicate fallthrough from mode if not specified in UI
				if (trigger.has_front) frontComp = trigger.front;
				if (trigger.has_case_comp) caseComp = trigger.case_comp;
				triggered = true;
			} else {
				break;
			}
		}
	}

	// TODO: stop led timers individually depending on if pattern set.
	//       Would only apply to RGB patterns for front LED, bulb uses chip tick interrupt

	// Update Front (Bulb and RGB)
	if (state.modeManager->currentMode.has_front || triggered) {
		if (frontComp.pattern.type == PATTERN_TYPE_SIMPLE && frontComp.pattern.data.simple.changeAt_count > 0) {
			SimpleOutput output = getOutputFromSimplePattern(&frontComp.pattern.data.simple, ms);
			if (output.type == BULB) {
				if (output.data.bulb == high) {
					state.writeBulbLedPin(1);
				} else {
					state.writeBulbLedPin(0);
				}
			} else {
				// TODO: RGB
				state.writeBulbLedPin(0);
			}
		}
	} else {
		state.writeBulbLedPin(0);
	}

	// Update Case (RGB only)
	// Don't update case led during button input, button input task uses case led for status
	if (!isEvaluatingButtonPress(state.button)) {
		if (state.modeManager->currentMode.has_case_comp || triggered) {
			if (caseComp.pattern.type ==  PATTERN_TYPE_SIMPLE && caseComp.pattern.data.simple.changeAt_count > 0) {
				SimpleOutput output = getOutputFromSimplePattern(&caseComp.pattern.data.simple, ms);
				if (output.type == RGB) {
					rgbShowUserColor(state.caseLed, output.data.rgb.r, output.data.rgb.g, output.data.rgb.b);
				} else {
					// TODO: BULB
				}
			}
		} else {
			// Default to off if no case component
			rgbShowUserColor(state.caseLed, 0, 0, 0);
		}
	}
}

// TODO: Rate of chipTick interrupt should be configurable
void chipTickInterrupt() {
	state.chipTick++;
	float millisPerTick = state.getMillisecondsPerChipTick();
	uint32_t ms = (uint32_t)(state.chipTick * millisPerTick);
	updateMode(ms);
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
