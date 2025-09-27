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
#include "mc3479.h"

static const uint8_t fakeOffModeIndex = 255;

static volatile uint16_t chipTick = 0;
static volatile uint8_t modeCount;
static volatile BulbMode currentMode;
static volatile bool clickStarted = false;
static volatile bool readChargerNow = false;

static uint16_t minutesUntilAutoOff;
static uint16_t minutesUntilLockAfterAutoOff;
static volatile uint32_t ticksSinceLastUserActivity = 0;

static RGB *caseLed;
static BQ25180 *chargerIC;
static MC3479 *accel;
static WriteToUsbSerial *writeUsbSerial;
static void (*enterDFU)();
static uint8_t (*readButtonPin)();
static void (*writeBulbLedPin)(uint8_t state);
static MillisForElapsedChipTicks *millisForElapsedChipTicks;
static void (*startLedTimers)(); // TODO: split bulb timer and rgb timer control into
static void (*stopLedTimers)();  //       different functions. Move rgb timers to RGB

static const char *fakeOffMode = "{\"command\":\"writeMode\",\"index\":255,\"mode\":{\"name\":\"fakeOff\",\"color\":\"#000000\",\"waveform\":{\"name\":\"off\",\"totalTicks\":1,\"changeAt\":[{\"tick\":0,\"output\":\"low\"}]}}}";
static const char *defaultMode = "{\"command\":\"writeMode\",\"index\":0,\"mode\":{\"name\":\"full on\",\"color\":\"#000000\",\"waveform\":{\"name\":\"on\",\"totalTicks\":1,\"changeAt\":[{\"tick\":0,\"output\":\"high\"}]}}}";

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
		char flashReadBuffer[1024];
		readBulbModeWithBuffer(modeIndex, mode, flashReadBuffer);
	}
}

static void setCurrentMode(BulbMode *mode) {
	currentMode = *mode;

	if (currentMode.modeIndex == fakeOffModeIndex) {
		stopLedTimers();
	} else {
		startLedTimers();
	}

	if (currentMode.triggerCount == 0) {
		mc3479Disable(accel);
	} else {
		mc3479Enable(accel);
	}
}

static void setCurrentModeIndex(uint8_t modeIndex) {
	BulbMode mode;
	readBulbMode(modeIndex, &mode);
	setCurrentMode(&mode);
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
		*settings = input.settings;
	}
}

static void readSettings(ChipSettings *settings) {
	char flashReadBuffer[1024];
	readSettingsWithBuffer(settings, flashReadBuffer);
}

static void shutdownFake() {
	setCurrentModeIndex(fakeOffModeIndex);
	if (getChargingState(chargerIC) != notConnected) {
		startLedTimers();
	}
}

void configureChipState(
		BQ25180 *_chargerIC,
		MC3479 *_accel,
		RGB *_caseLed,
		WriteToUsbSerial *_writeUsbSerial,
		void (*_enterDFU)(),
		uint8_t (*_readButtonPin)(),
		void (*_writeBulbLedPin)(uint8_t state),
		MillisForElapsedChipTicks *_millisForElapsedChipTicks,
		void (*_startLedTimers)(),
		void (*_stopLedTimers)()
) {
	caseLed = _caseLed;
	chargerIC = _chargerIC;
	accel = _accel;
	writeUsbSerial = _writeUsbSerial;
	enterDFU = _enterDFU;
	readButtonPin = _readButtonPin;
	writeBulbLedPin = _writeBulbLedPin;
	millisForElapsedChipTicks = _millisForElapsedChipTicks;
	startLedTimers = _startLedTimers;
	stopLedTimers = _stopLedTimers;

	enum ChargeState state = getChargingState(chargerIC);

	if (state == notConnected) {
		setCurrentModeIndex(0);
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
	rgbShowNoColor(caseLed);
	// Timers interrupts needed to properly detect input.
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

static void buttonInputTask(uint16_t tick) {
	static int16_t clickStartTick = 0;
	
	if (hasClickStarted() && clickStartTick == 0) {
		clickStartTick = tick;
	}

	uint16_t elapsedMillis = 0;
	if (clickStartTick != 0) {
		elapsedMillis = millisForElapsedChipTicks(tick - clickStartTick);
	}

	uint8_t state = readButtonPin();
	bool buttonCurrentlyDown = state == 0;

	if (buttonCurrentlyDown) {
		if (elapsedMillis > 2000 && elapsedMillis < 2100) {
			rgbShowLocked(caseLed);
		} else if (elapsedMillis > 1000 && elapsedMillis < 1100) {
			rgbShowShutdown(caseLed);
		}
	}

	if (!buttonCurrentlyDown && elapsedMillis > 20) {
		enum ButtonResult buttonState = clicked;
		if (elapsedMillis > 2000) {
			buttonState = lockOrHardwareReset;
		} else if (elapsedMillis > 1000) {
			buttonState = shutdown;
		}

		switch (buttonState) {
		case clicked:
			rgbShowSuccess(caseLed);
			uint8_t newModeIndex = currentMode.modeIndex + 1;
			if (newModeIndex >= modeCount) {
				newModeIndex = 0;
			}
			setCurrentModeIndex(newModeIndex);
			break;
		case shutdown:
			shutdownFake();
			break;
		case lockOrHardwareReset:
			lock();
			break;
		}

		clickStartTick = 0;
		setClickEnded();
	}
}

static void showChargingState(enum ChargeState state) {
	if (hasClickStarted() || currentMode.modeIndex != fakeOffModeIndex) {
		// don't show charging during button input, or when a mode is in use while plugged in, will still charge
		return;
	}

	switch (state) {
	case notConnected:
		// do nothing
		break;
	case notCharging:
		rgbShowNotCharging(caseLed);
		break;
	case constantCurrent:
		rgbShowConstantCurrentCharging(caseLed);
		break;
	case constantVoltage:
		rgbShowConstantVoltageCharging(caseLed);
		break;
	case done:
		rgbShowDoneCharging(caseLed);
		break;
	}
}

// TODO: use milliseconds elapsed instead of tick count. see millisForElapsedChipTicks
static void chargerTask(uint16_t tickCount) {
	static enum ChargeState chargingState = notConnected;

	uint8_t previousState = chargingState;
	if (tickCount % 1024 == 0) {
		// TODO: Do I need to configure periodically? There should be i2c communication 
		// 		 from reads, should not reset to defaults.
		// configureChargerIC(chargerIC);

		// char registerJson[256];
		// readAllRegistersJson(chargerIC, registerJson);
		// writeUsbSerial(0, registerJson, strlen(registerJson));
		// printAllRegisters(chargerIC);
		chargingState = getChargingState(chargerIC);
	}

	if (tickCount % 256 == 0) {
		showChargingState(chargingState);
	}

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
			showChargingState(state);
		}
	}
}

void stateTask() {
	buttonInputTask(chipTick);
	rgbTask(caseLed, chipTick);
	mc3479Task(accel, chipTick);
	chargerTask(chipTick);
}

void handleChargerInterrupt() {
	readChargerNow = 1;
}

// called from chipTick interrupt
static void updateMode() {
	static uint8_t modeInterruptCount = 0;
	static uint8_t nextTickInMode = 0;
	static uint8_t currentChangeIndex = 0;

	Waveform waveform;

	// TODO: check for configurable trigger threshold
	if (isOverThreshold(accel, 0.3f) && currentMode.triggerCount > 0) {
		AccelTrigger trigger = currentMode.triggers[0];
		waveform = trigger.waveform;

		if (!hasClickStarted()) {
			rgbShowUserColor(caseLed, trigger.red, trigger.green, trigger.blue);
		}
	} else {
		waveform = currentMode.waveform;
		if (!hasClickStarted()) {
			rgbShowUserColor(caseLed, currentMode.red, currentMode.green, currentMode.blue);
		}
	}

	if (waveform.totalTicks <= modeInterruptCount) {
		modeInterruptCount = 0;
		currentChangeIndex = 0;
		nextTickInMode = 0;
	}

	if (modeInterruptCount == nextTickInMode) {
		if (waveform.changeAt[currentChangeIndex].output == high) {
			writeBulbLedPin(1);
		} else {
			writeBulbLedPin(0);
		}

		if (currentChangeIndex + 1 < waveform.numChanges) {
			nextTickInMode = waveform.changeAt[currentChangeIndex + 1].tick;
		}

		currentChangeIndex++;
	}

	modeInterruptCount++;
}

// TODO: Rate of chipTick interrupt should be configurable. Places
//	 that use chipTick should be able to resolve to time in 
//   milliseconds for consistent behavior with different timer rates
void chipTickInterrupt() {
	chipTick++;
	updateMode();
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
		 setCurrentMode(&mode);
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
		rgbShowSuccess(caseLed);
	}
}
