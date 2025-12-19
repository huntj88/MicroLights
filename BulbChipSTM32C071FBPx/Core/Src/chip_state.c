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
    void (*enterDFU)();
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
		void (*_enterDFU)(),
//		uint8_t (*_readButtonPin)(),
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
	state.enterDFU = _enterDFU;
//	state.readButtonPin = _readButtonPin;
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

//// TODO: move to button input file
//void setClickStarted() {
//	clickStarted = true;
//	rgbShowNoColor(caseLed);
//	// Timers interrupts needed to properly detect input, chipTickTimer is part of led timers currently.
//	startLedTimers();
//}
//
//// TODO: move to button input file
//static void setClickEnded() {
//	clickStarted = false;
//	ticksSinceLastUserActivity = 0;
//}

// TODO: move to button input file
//static uint8_t hasClickStarted() {
//	return clickStarted;
//}

// TODO: move to charger file
//static void lock() {
//	enum ChargeState state = getChargingState(chargerIC);
//	if (state == notConnected) {
//		enableShipMode(chargerIC);
//	} else {
//		hardwareReset(chargerIC);
//	}
//}

//enum ButtonResult {
//	ignore,
//	clicked,
//	shutdown,
//	lockOrHardwareReset // hardware reset occurs when usb is plugged in
//};

// TODO: extract ButtonResult / button click logic to different file, return enum via pointer, update state based on enum. Different enum pointer for rgb hold state?
//static void buttonInputTask(uint16_t tick, float millisPerTick) {
//	static int16_t clickStartTick = 0;
//
//	if (hasClickStarted() && clickStartTick == 0) {
//		clickStartTick = tick;
//	}
//
//	uint16_t elapsedMillis = 0;
//	if (clickStartTick != 0) {
//		uint16_t elapsedTicks = tick - clickStartTick;
//		elapsedMillis = elapsedTicks * millisPerTick;
//	}
//
//	uint8_t state = readButtonPin();
//	bool buttonCurrentlyDown = state == 0;
//
//	if (buttonCurrentlyDown) {
//		if (elapsedMillis > 2000 && elapsedMillis < 2100) {
//			rgbShowLocked(caseLed);
//		} else if (elapsedMillis > 1000 && elapsedMillis < 1100) {
//			rgbShowShutdown(caseLed);
//		}
//	}
//
//	if (!buttonCurrentlyDown && elapsedMillis > 20) {
//		enum ButtonResult buttonState = clicked;
//		if (elapsedMillis > 2000) {
//			buttonState = lockOrHardwareReset;
//		} else if (elapsedMillis > 1000) {
//			buttonState = shutdown;
//		}
//
//		switch (buttonState) {
//		case ignore:
//			break;
//		case clicked:
//			rgbShowSuccess(caseLed);
//			uint8_t newModeIndex = currentModeIndex + 1;
//			if (newModeIndex >= modeCount) {
//				newModeIndex = 0;
//			}
//			setCurrentModeIndex(newModeIndex);
//			const char *blah = "clicked\n";
//			writeUsbSerial(0, blah, strlen(blah));
//			break;
//		case shutdown:
//			shutdownFake();
//			break;
//		case lockOrHardwareReset:
////			lock(); // TODO
//			break;
//		}
//
//		clickStartTick = 0;
//		setClickEnded();
//	}
//}

// TODO: move to charger file, add case rgbLed device as charger dependency
// static void showChargingState(enum ChargeState state) {
// 	// TODO: after moving, disabled variable to handle this case, update from chipState
// 	if (hasClickStarted() || currentModeIndex != fakeOffModeIndex) {
// 		// don't show charging during button input, or when a mode is in use while plugged in, will still charge
// 		return;
// 	}

// 	switch (state) {
// 	case notConnected:
// 		// do nothing
// 		break;
// 	case notCharging:
// 		rgbShowNotCharging(caseLed);
// 		break;
// 	case constantCurrent:
// 		rgbShowConstantCurrentCharging(caseLed);
// 		break;
// 	case constantVoltage:
// 		rgbShowConstantVoltageCharging(caseLed);
// 		break;
// 	case done:
// 		rgbShowDoneCharging(caseLed);
// 		break;
// 	}
// }

// // TODO: move to charger file
// static void chargerTask(uint16_t tick, float millisPerTick) {
// 	static enum ChargeState chargingState = notConnected;
// 	static uint16_t checkedAtTick = 0;

// 	uint8_t previousState = chargingState;
// 	uint16_t elapsedMillis = 0;

// 	if (checkedAtTick != 0) {
// 		uint16_t elapsedTicks = tick - checkedAtTick;
// 		elapsedMillis = elapsedTicks * millisPerTick;
// 	}

// 	// charger i2c watchdog timer will reset if not communicated
// 	// with for 40 seconds, and 15 seconds after plugged in.
// 	if (elapsedMillis > 30000 || checkedAtTick == 0) {
// 		// char registerJson[256];
// 		// readAllRegistersJson(chargerIC, registerJson);
// 		// writeUsbSerial(0, registerJson, strlen(registerJson));
// 		// printAllRegisters(chargerIC);

// 		chargingState = getChargingState(chargerIC);
// 		checkedAtTick = tick;
// 	}

// 	// flash charging state to user every second
// 	if (chargingState != notConnected && elapsedMillis % 1000 >= 1000 - millisPerTick) {
// 		showChargingState(chargingState);
// 	}

// 	if (readChargerNow) {
// 		readChargerNow = false;
// 		enum ChargeState state = getChargingState(chargerIC);

// 		bool wasDisconnected = previousState != notConnected && state == notConnected;
// 		if (tick != 0 && wasDisconnected && currentModeIndex == fakeOffModeIndex) {
// 			// if in fake off mode and power is unplugged, put into ship mode
// 			lock();
// 		}

// 		bool wasConnected = previousState == notConnected && state != notConnected;
// 		if (wasConnected) {
// 			startLedTimers(); // show charging status led
// 			showChargingState(state);
// 		}
// 	}
// }

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

// TODO: move to charger file
// void handleChargerInterrupt() {
// 	readChargerNow = 1;
// }

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
//				lock(); // TODO
			} else {
				state.ticksSinceLastUserActivity = 0; // restart timer to transition from fakeOff to shipMode
				shutdownFake();
			}
		}
	}
}

void chip_state_enter_dfu() {
	state.enterDFU();
}

void chip_state_write_serial(const char *msg) {
	state.writeUsbSerial(0, msg, strlen(msg));
}

void chip_state_show_success() {
	rgbShowSuccess(state.caseLed);
}
