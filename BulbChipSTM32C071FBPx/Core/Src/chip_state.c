/*
 * chip_state.c
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#include <stdint.h>
#include "chip_state.h"
#include "storage.h"
#include "rgb.h"

static const uint8_t fakeOffModeIndex = 255;

// TODO: don't read flash every time mode changes?, can be cached
static volatile uint8_t modeCount = 0;
static volatile BulbMode currentMode;
static volatile uint8_t clickStarted = 0;
static volatile uint8_t readChargerNow = 0;

// TODO:  make auto off cli configurable
static uint16_t minutesUntilAutoOff = 90;
static uint16_t minutesUntilLockAfterAutoOff = 10;
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

		if (input.mode.numChanges > 0 && input.mode.totalTicks > 0) {
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
	readSettingsFromFlash(flashReadBuffer, 1024);
	parseJson(flashReadBuffer, 1024, &input);
	*settings = input.settings;
}

// TODO: create fake off mode
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

	uint8_t state = getChargingState(chargerIC);
	BulbMode mode;
	if (state == NOT_CONNECTED) {
		readBulbMode(0, &mode);
	} else {
		readBulbMode(fakeOffModeIndex, &mode);
	}
	currentMode = mode;

	ChipSettings settings;
	readSettings(&settings);
	modeCount = settings.modeCount;
}

void setClickStarted() {
	clickStarted = 1;
}

static void setClickEnded() {
	clickStarted = 0;
	ticksSinceLastUserActivity = 0;
	char *blah = "clicked\n";
	writeUsbSerial(0, blah, strlen(blah));
}

static uint8_t hasClickStarted() {
	return clickStarted;
}
static void shutdownFake() {
	BulbMode mode;
	readBulbMode(fakeOffModeIndex, &mode);
	currentMode = mode;
}

static void lock() {
	uint8_t state = getChargingState(chargerIC);
	if (state == NOT_CONNECTED) {
		enableShipMode(chargerIC);
	} else {
		hardwareReset(chargerIC);
	}
}

static void handleButtonInput() {
	// Button states
	// 0: confirming click
	// 1: clicked
	// 2: shutdown
	// 3: lock and shutdown, or hardware reset if plugged into usb
	static uint8_t buttonState = 0;
	static int16_t buttonDownCounter = 0;

	if (hasClickStarted()) {
		uint8_t state = readButtonPin();
		uint8_t buttonCurrentlyDown = state == 0;

		if (buttonCurrentlyDown) {
			buttonDownCounter += 1;
			if (buttonDownCounter > 2 && buttonState == 0) {
				buttonState = 1;// will register as button click
			} else if (buttonDownCounter > 400 && buttonState == 1) {
				buttonState = 2; // shutdown
				showShutdown();
			} else if (buttonDownCounter > 800 && buttonState == 2) {
				buttonState = 3; // shutdown and lock
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
				if (buttonState == 0) {
					// ignore, probably pulse from chargerIC?
				} else if (buttonState == 1) {
					// Button clicked and released.
					showSuccess();
					uint8_t newModeIndex = currentMode.modeIndex + 1;
					if (newModeIndex > modeCount) {
						newModeIndex = 0;
					}

					BulbMode newMode;
					readBulbMode(newModeIndex, &newMode);
					currentMode = newMode;
				} else if (buttonState == 2 || buttonState == 4) {
					shutdownFake();
				} else if (buttonState == 3) {
					lock();
				}
				setClickEnded();
				buttonState = 0;
			}
		}
	}
}

static void showChargingState(uint8_t state) {
	if (state == NOT_CONNECTED) {

	} else if (state == NOT_CHARGING) {
		showNotCharging();
	} else if (state == CONSTANT_CURRENT_CHARGING) {
		showConstantCurrentCharging();
	} else if (state == CONSTANT_VOLTAGE_CHARGING) {
		showConstantVoltageCharging();
	} else if (state == DONE_CHARGING) {
		showDoneCharging();
	}
}




static void chargerTask(uint16_t tickCount) {
	static uint8_t chargingState = 0;

	uint8_t previousState = chargingState;
	if (tickCount % 1024 == 0) {
		configureChargerIC(chargerIC);
		printAllRegisters(chargerIC);
		chargingState = getChargingState(chargerIC);
	}

	if (tickCount % 256 == 0) {
		showChargingState(chargingState);
	}

	// TODO: charger alternates between status's too fast when transitioning from one charging mode to another
	if (readChargerNow) {
		readChargerNow = 0;
		uint8_t state = getChargingState(chargerIC);

		uint8_t wasUnplugged = previousState != NOT_CONNECTED && state == NOT_CONNECTED;
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

void autoOffTimerInterrupt() {
	if (getChargingState(chargerIC) == NOT_CONNECTED) {
		ticksSinceLastUserActivity++;

		uint16_t ticksUntilAutoOff = minutesUntilAutoOff * 60 / 10; // auto off timer running at 0.1hz
		uint8_t enterLock = ticksSinceLastUserActivity > ticksUntilAutoOff;
		if (currentMode.modeIndex == fakeOffModeIndex) {
			uint16_t ticksUntilLockAfterAutoOff = minutesUntilLockAfterAutoOff * 60 / 10;
			enterLock = ticksSinceLastUserActivity > ticksUntilLockAfterAutoOff;
		}

		if (enterLock) {
			if (currentMode.modeIndex == fakeOffModeIndex) {
				lock();
			} else {
				ticksSinceLastUserActivity = 0; // restart timer to transition from fakeOff to shipMode
				shutdownFake();
				stopLedTimers();
				// TODO: make sure outputs are low. Timer can be turned off before led pins set low from mode
			}
		}
	}
}

void handleJson(uint8_t buf[], uint32_t count) {
	CliInput input;
	parseJson(buf, count, &input);

	if (input.parsedType == 1) {
		BulbMode mode = input.mode;
		if (mode.numChanges > 0 && mode.totalTicks > 0) {
			writeBulbModeToFlash(mode.modeIndex, buf, input.jsonLength);
			currentMode = mode;
			showSuccess();
		}
	} else if (input.parsedType == 2) {
		ChipSettings settings = input.settings;
		writeSettingsToFlash(buf, input.jsonLength);
		modeCount = settings.modeCount;
		showSuccess();
	} else if (input.parsedType == 3) {
		enterDFU();
	}
}
