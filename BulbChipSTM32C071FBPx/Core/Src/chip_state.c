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

static const char *fakeOffMode = "{\"command\":\"writeMode\",\"index\":255,\"mode\":{\"name\":\"default\",\"totalTicks\":1,\"changeAt\":[{\"tick\":0,\"output\":\"low\"}]}}";
static const char *defaultMode = "{\"command\":\"writeMode\",\"index\":0,\"mode\":{\"name\":\"default\",\"totalTicks\":1,\"changeAt\":[{\"tick\":0,\"output\":\"high\"}]}}";

static void readBulbModeWithBuffer(uint8_t modeIndex, BulbMode *mode, char *buffer) {
	CliInput input;
	readBulbModeFromFlash(modeIndex, buffer, 1024);
	parseJson(buffer, 1024, &input);

	if (input.parsedType == parseWriteMode) {
		// successfully read from flash
		*mode = input.mode;
	} else {
		// fallback to default
		parseJson(defaultMode, 1024, &input);
		*mode = input.mode;
	}
}

static void readBulbMode(uint8_t modeIndex, BulbMode *mode) {
	if (modeIndex == fakeOffModeIndex) {
		CliInput input;
		parseJson(fakeOffMode, 1024, &input);
		*mode = input.mode;
	} else {
		startLedTimers();
		char flashReadBuffer[1024];
		readBulbModeWithBuffer(modeIndex, mode, flashReadBuffer);
	}
}

static void readSettingsWithBuffer(ChipSettings *settings, char *buffer) {
	CliInput input;
	// set some defaults
	settings->modeCount = 0;
	settings->minutesUntilAutoOff = 90;
	settings->minutesUntilLockAfterAutoOff = 10;

	readSettingsFromFlash(buffer, 1024);
	parseJson(buffer, 1024, &input);

	if (input.parsedType == parseWriteSettings) {
		settings->modeCount = input.settings.modeCount;
		settings->minutesUntilAutoOff = input.settings.minutesUntilAutoOff;
		settings->minutesUntilLockAfterAutoOff = input.settings.minutesUntilLockAfterAutoOff;
	}
}

static void readSettings(ChipSettings *settings) {
	char flashReadBuffer[1024];
	readSettingsWithBuffer(settings, flashReadBuffer);
}

static void shutdownFake() {
	BulbMode mode;
	readBulbMode(fakeOffModeIndex, &mode);
	currentMode = mode;

	if (getChargingState(chargerIC) == notConnected) {
		stopLedTimers();
	} else {
		startLedTimers();
	}
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
	startLedTimers();
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

// Start the timers if they are not already started until the click is over at least to capture the button press properly
// See setClickStarted();
// After, the timers may be stopped depending on click result
static void handleButtonInput() {
	static enum ButtonResult buttonState = ignore;
	static int16_t buttonDownCounter = 0;

	if (hasClickStarted()) {
		uint8_t state = readButtonPin();
		bool buttonCurrentlyDown = state == 0;

		if (buttonCurrentlyDown) {
			buttonDownCounter += 1;
			if (buttonDownCounter > 2 && buttonState == ignore) {
				buttonState = clicked;
			} else if (buttonDownCounter > 400 && buttonState == clicked) {
				buttonState = shutdown;
				showShutdown();
			} else if (buttonDownCounter > 800 && buttonState == shutdown) {
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
				switch (buttonState) {
				case ignore:
					if (currentMode.modeIndex == fakeOffModeIndex && getChargingState(chargerIC) == notConnected) {
						stopLedTimers();
					}
					break;
				case clicked:
					showSuccess();
					uint8_t newModeIndex = currentMode.modeIndex + 1;
					if (newModeIndex >= modeCount) {
						newModeIndex = 0;
					}

					BulbMode newMode;
					readBulbMode(newModeIndex, &newMode);
					currentMode = newMode;
					break;
				case shutdown:
					shutdownFake();
					break;
				case lockOrHardwareReset:
					lock();
					break;
				}
				setClickEnded();
				buttonState = ignore;
			}
		}
	}
}

static void showChargingState(enum ChargeState state) {
	switch (state) {
	case notConnected:
		// do nothing
		break;
	case notCharging:
		showNotCharging();
		break;
	case constantCurrent:
		showConstantCurrentCharging();
		break;
	case constantVoltage:
		showConstantVoltageCharging();
		break;
	case done:
		showDoneCharging();
		break;
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

		bool wasDisconnected = previousState != notConnected && state == notConnected;
		if (tickCount != 0 && wasDisconnected && currentMode.modeIndex == fakeOffModeIndex) {
			// if in fake off mode and power is unplugged, put into ship mode
			lock();
		}

		bool wasConnected = previousState == notConnected && state != notConnected;
		if (wasConnected) {
			startLedTimers(); // show charging status led
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
			}
		}
	}
}

void handleJson(uint8_t buf[], uint32_t count) {
	CliInput input;
	parseJson(buf, count, &input);

	switch (input.parsedType) {
	case parseError: {
		char error[] = "{\"error\":\"unable to parse json\"}\n";
		writeUsbSerial(0, error, strlen(error));
		break;
	}
	case parseWriteMode: {
		BulbMode mode = input.mode;
		writeBulbModeToFlash(mode.modeIndex, buf, input.jsonLength);
		currentMode = mode;
		break;
	}
	case parseReadMode: {
		char flashReadBuffer[1024];
		BulbMode mode;
		readBulbModeWithBuffer(input.mode.modeIndex, &mode, flashReadBuffer);
		uint16_t len = strlen(flashReadBuffer);
		flashReadBuffer[len] = '\n';
		flashReadBuffer[len + 1] = '\0';
		writeUsbSerial(0, flashReadBuffer, strlen(flashReadBuffer));
		break;
	}
	case parseWriteSettings: {
		ChipSettings settings = input.settings;
		writeSettingsToFlash(buf, input.jsonLength);
		modeCount = settings.modeCount;
		minutesUntilAutoOff = settings.minutesUntilAutoOff;
		minutesUntilLockAfterAutoOff = settings.minutesUntilLockAfterAutoOff;
		break;
	}
	case parseReadSettings: {
		char flashReadBuffer[1024];
		ChipSettings settings;
		readSettingsWithBuffer(&settings, flashReadBuffer);
		uint16_t len = strlen(flashReadBuffer);
		flashReadBuffer[len] = '\n';
		flashReadBuffer[len + 1] = '\0';
		writeUsbSerial(0, flashReadBuffer, strlen(flashReadBuffer));
		break;
	}
	case parseDfu: {
		enterDFU();
		break;
	}}

	if (input.parsedType != parseError) {
		showSuccess();
	}
}
