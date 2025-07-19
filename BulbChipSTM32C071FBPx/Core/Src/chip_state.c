/*
 * chip_state.c
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#include <stdint.h>
#include <stdbool.h>
#include "chip_state.h"
#include "storage.h"
#include "rgb.h"
#include "bulb_json.h"

static const uint8_t fakeOffModeIndex = 255;

static volatile uint8_t modeCount;
static volatile BulbMode currentMode;
static volatile bool clickStarted = false;
static volatile bool readChargerNow = false;

static uint16_t minutesUntilAutoOff;
static uint16_t minutesUntilLockAfterAutoOff;
volatile static uint32_t ticksSinceLastUserActivity = 0;

static BQ25180 *chargerIC;
static WriteToUsbSerial *writeUsbSerial;
static void (*enterDFU)();
static uint8_t (*readButtonPin)();
static void (*writeBulbLedPin)(uint8_t state);
static void (*startLedTimers)();
static void (*stopLedTimers)();

static const char *fakeOffMode = "{\"command\":\"setMode\",\"index\":255,\"mode\":{\"name\":\"default\",\"totalTicks\":1,\"changeAt\":[{\"tick\":0,\"output\":\"low\"}]}}";
static const char *defaultMode = "{\"command\":\"setMode\",\"index\":0,\"mode\":{\"name\":\"default\",\"totalTicks\":1,\"changeAt\":[{\"tick\":0,\"output\":\"high\"}]}}";

static void readBulbMode(uint8_t modeIndex, BulbMode *mode) {
	startLedTimers();
	CliInput input;
	if (modeIndex == fakeOffModeIndex) {
		parseJson(fakeOffMode, 1024, &input);
		*mode = input.mode;
	} else {
		char flashReadBuffer[1024];
		readBulbModeFromFlash(modeIndex, flashReadBuffer, 1024);
		parseJson(flashReadBuffer, 1024, &input);

		if (input.parsedType == parseMode && input.mode.numChanges > 0 && input.mode.totalTicks > 0) {
			// successfully read from flash
			*mode = input.mode;
		} else {
			// fallback to default
			parseJson(defaultMode, 1024, &input);
			*mode = input.mode;
		}
	}
}

static void readSettings(ChipSettings *settings) {
	CliInput input;
	char flashReadBuffer[1024];

	// set some defaults
	settings->modeCount = 0;
	settings->minutesUntilAutoOff = 90;
	settings->minutesUntilLockAfterAutoOff = 10;

	readSettingsFromFlash(flashReadBuffer, 1024);
	parseJson(flashReadBuffer, 1024, &input);

	if (input.parsedType == parseSettings) {
		*settings = input.settings;
	}
}

static void shutdownFake() {
	BulbMode mode;
	readBulbMode(fakeOffModeIndex, &mode);
	currentMode = mode;
}

void configureChipState(
		BQ25180 *_chargerIC,
		WriteToUsbSerial *_writeUsbSerial,
		void (*_enterDFU)(),
		uint8_t (*_readButtonPin)(),
		void (*_writeBulbLedPin)(uint8_t state),
		void (*_startLedTimers)(),
		void (*_stopLedTimers)()
) {
	chargerIC = _chargerIC;
	writeUsbSerial = _writeUsbSerial;
	enterDFU = _enterDFU;
	readButtonPin = _readButtonPin;
	writeBulbLedPin = _writeBulbLedPin;
	startLedTimers = _startLedTimers;
	stopLedTimers = _stopLedTimers;

	enum ChargeState state = getChargingState(chargerIC);

	if (state == notConnected) {
		BulbMode mode;
		readBulbMode(0, &mode);
		currentMode = mode;
	} else {
		shutdownFake();
	}

	ChipSettings settings;
	readSettings(&settings);
	modeCount = settings.modeCount;
	minutesUntilAutoOff = settings.minutesUntilAutoOff;
	minutesUntilLockAfterAutoOff = settings.minutesUntilLockAfterAutoOff;
}

void setClickStarted() {
	clickStarted = true;
}

static void setClickEnded() {
	clickStarted = false;
	ticksSinceLastUserActivity = 0;
	char *blah = "clicked\n";
	writeUsbSerial(0, blah, strlen(blah));
}

static uint8_t hasClickStarted() {
	return clickStarted;
}

static void lock() {
	enum ChargeState state = getChargingState(chargerIC);
	if (state == notConnected) {
		enableShipMode(chargerIC);
	} else {
		hardwareReset(chargerIC);
	}
}

enum ButtonResult {
	ignore,
	clicked,
	shutdown,
	lockOrHardwareReset // hardware reset occurs when usb is plugged in
};

static void handleButtonInput() {
	static enum ButtonResult buttonState = ignore;
	static int16_t buttonDownCounter = 0;

	if (hasClickStarted()) {
		uint8_t state = readButtonPin();
		bool buttonCurrentlyDown = state == 0;

		if (buttonCurrentlyDown) {
			buttonDownCounter += 1;
			if (buttonDownCounter > 2 && buttonState == 0) {
				buttonState = clicked;
			} else if (buttonDownCounter > 400 && buttonState == 1) {
				buttonState = shutdown;
				showShutdown();
			} else if (buttonDownCounter > 800 && buttonState == 2) {
				buttonState = lockOrHardwareReset;
				showLocked();
			}

			// prevent long holds from increasing time for button action to start
			if (buttonDownCounter > 800) {
				buttonDownCounter = 800;
			}
		} else {
			buttonDownCounter -= 500; // Large decrement to allow any hold time to "discharge" quickly.
			if (buttonDownCounter < -1000) {
				buttonDownCounter = 0;
				if (buttonState == ignore) {
					// ignore, probably pulse from chargerIC?
				} else if (buttonState == clicked) {
					// Button clicked and released.
					showSuccess();
					uint8_t newModeIndex = currentMode.modeIndex + 1;
					if (newModeIndex >= modeCount) {
						newModeIndex = 0;
					}

					BulbMode newMode;
					readBulbMode(newModeIndex, &newMode);
					currentMode = newMode;
				} else if (buttonState == shutdown) {
					shutdownFake();
				} else if (buttonState == lockOrHardwareReset) {
					lock();
				}
				setClickEnded();
				buttonState = ignore;
			}
		}
	}
}

static void showChargingState(enum ChargeState state) {
	if (state == notConnected) {

	} else if (state == notCharging) {
		showNotCharging();
	} else if (state == constantCurrent) {
		showConstantCurrentCharging();
	} else if (state == constantVoltage) {
		showConstantVoltageCharging();
	} else if (state == done) {
		showDoneCharging();
	}
}

static void chargerTask(uint16_t tickCount) {
	static enum ChargeState chargingState = notConnected;

	uint8_t previousState = chargingState;
	if (tickCount % 1024 == 0) {
		configureChargerIC(chargerIC);

		char registerJson[256];
		readAllRegistersJson(chargerIC, registerJson);
		writeUsbSerial(0, registerJson, strlen(registerJson));
//		printAllRegisters(chargerIC);
		chargingState = getChargingState(chargerIC);
	}

	if (tickCount % 256 == 0) {
		showChargingState(chargingState);
	}

	// TODO: charger alternates between status's too fast when transitioning from one charging mode to another
	if (readChargerNow) {
		readChargerNow = false;
		enum ChargeState state = getChargingState(chargerIC);

		bool wasUnplugged = previousState != notConnected && state == notConnected;
		if (tickCount != 0 && wasUnplugged && currentMode.modeIndex == fakeOffModeIndex) {
			// if in fake off mode and power is unplugged, put into ship mode
			lock();
		}

		if (chargingState != state) {
			showChargingState(state);
			chargingState = state;
		}
	}
}

void stateTask() {
	static uint16_t tickCount = 0;

	chargerTask(tickCount);
	handleButtonInput();

	tickCount++;
}

void handleChargerInterrupt() {
	readChargerNow = 1;
}

void modeTimerInterrupt() {
	static uint8_t modeInterruptCount = 0;
	static uint8_t nextTickInMode = 0;
	static uint8_t currentChangeIndex = 0;

	if (currentMode.totalTicks <= modeInterruptCount) {
		modeInterruptCount = 0;
		currentChangeIndex = 0;
		nextTickInMode = 0;
	}

	if (modeInterruptCount == nextTickInMode) {
		if (currentMode.changeAt[currentChangeIndex].output == high) {
			writeBulbLedPin(1);
		} else {
			writeBulbLedPin(0);
		}

		if (currentChangeIndex + 1 < currentMode.numChanges) {
			nextTickInMode = currentMode.changeAt[currentChangeIndex + 1].tick;
		}

		currentChangeIndex++;
	}

	modeInterruptCount++;
}

// Auto off timer running at 0.1 hz
// 12 megahertz / 65535 / 1831 = 0.1 hz
void autoOffTimerInterrupt() {
	if (getChargingState(chargerIC) == notConnected) {
		ticksSinceLastUserActivity++;

		uint16_t ticksUntilAutoOff = minutesUntilAutoOff * 60 / 10; // auto off timer running at 0.1hz
		bool autoOffTimerDone = ticksSinceLastUserActivity > ticksUntilAutoOff;
		if (currentMode.modeIndex == fakeOffModeIndex) {
			uint16_t ticksUntilLockAfterAutoOff = minutesUntilLockAfterAutoOff * 60 / 10;
			autoOffTimerDone = ticksSinceLastUserActivity > ticksUntilLockAfterAutoOff;
		}

		if (autoOffTimerDone) {
			if (currentMode.modeIndex == fakeOffModeIndex) {
				lock();
			} else {
				ticksSinceLastUserActivity = 0; // restart timer to transition from fakeOff to shipMode
				shutdownFake();
				stopLedTimers();
			}
		}
	}
}

void handleJson(uint8_t buf[], uint32_t count) {
	CliInput input;
	parseJson(buf, count, &input);

	if (input.parsedType == parseMode) {
		BulbMode mode = input.mode;
		if (mode.numChanges > 0 && mode.totalTicks > 0) {
			writeBulbModeToFlash(mode.modeIndex, buf, input.jsonLength);
			currentMode = mode;
			showSuccess();
		}
	} else if (input.parsedType == parseSettings) {
		ChipSettings settings = input.settings;
		writeSettingsToFlash(buf, input.jsonLength);
		modeCount = settings.modeCount;
		minutesUntilAutoOff = settings.minutesUntilAutoOff;
		minutesUntilLockAfterAutoOff = settings.minutesUntilLockAfterAutoOff;

		showSuccess();
	} else if (input.parsedType == parseDfu) {
		enterDFU();
	}
}
