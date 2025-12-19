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
	enum ButtonResult buttonResult = buttonInputTask(state.button, state.chipTick, millisPerTick);
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

	rgbTask(state.caseLed, state.chipTick, millisPerTick);
	mc3479Task(state.accel, state.chipTick, millisPerTick);

	bool unplugLockEnabled = isFakeOff(state.modeManager);
	bool chargeLedEnabled = !isEvaluatingButtonPress(state.button) && isFakeOff(state.modeManager);
	chargerTask(state.chargerIC, state.chipTick, millisPerTick, unplugLockEnabled, chargeLedEnabled);
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

// TODO: pass in tick and millisPerTick to resolve time instead of using chipTick and getMillisecondsPerChipTick
static void updateMode() {
	static uint32_t modeMs = 0;
	
	modeMs += state.getMillisecondsPerChipTick();

	// Check triggers
	ModeComponent frontComp = state.modeManager->currentMode.front;
	ModeComponent caseComp = state.modeManager->currentMode.case_comp;
	bool triggered = false;

	if (state.modeManager->currentMode.has_accel && state.modeManager->currentMode.accel.triggers_count > 0) {
		// TODO: check for configurable trigger threshold
		// For now using hardcoded 0.3f, but should use trigger.threshold
		if (isOverThreshold(state.accel, 0.3f)) {
			// Use first trigger for now
			ModeAccelTrigger trigger = state.modeManager->currentMode.accel.triggers[0];

			// TODO: indicate fallthrough from mode if not specified in UI
			if (trigger.has_front) frontComp = trigger.front;
			if (trigger.has_case_comp) caseComp = trigger.case_comp;
			triggered = true;
		}
	}

	// Update Front (Bulb and RGB)
	if (state.modeManager->currentMode.has_front || (triggered && state.modeManager->currentMode.accel.triggers[0].has_front)) {
		if (frontComp.pattern.type == PATTERN_TYPE_SIMPLE && frontComp.pattern.data.simple.changeAt_count > 0) {
			SimpleOutput output = getOutputFromSimplePattern(&frontComp.pattern.data.simple, modeMs);
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
	// Don't show case led changes during button input, button input task uses case led for status
	if (!isEvaluatingButtonPress(state.button)) {
		if (state.modeManager->currentMode.has_case_comp || (triggered && state.modeManager->currentMode.accel.triggers[0].has_case_comp)) {
			if (caseComp.pattern.type ==  PATTERN_TYPE_SIMPLE && caseComp.pattern.data.simple.changeAt_count > 0) {
				SimpleOutput output = getOutputFromSimplePattern(&caseComp.pattern.data.simple, modeMs);
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

// TODO: Rate of chipTick interrupt should be configurable. Places
//	 that use chipTick should be able to resolve to time in 
//   milliseconds for consistent behavior with different timer rates
void chipTickInterrupt() {
	state.chipTick++;
	updateMode();
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
