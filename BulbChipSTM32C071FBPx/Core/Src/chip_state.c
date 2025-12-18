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
#include "device/mc3479.h"
#include "device/rgb_led.h"
#include "json/command_parser.h"
#include "json/mode_parser.h"
#include "storage.h"

static const uint8_t fakeOffModeIndex = 255;

static volatile uint16_t chipTick = 0;

static volatile uint8_t modeCount;
static volatile Mode currentMode;
static volatile uint8_t currentModeIndex;
static volatile bool clickStarted = false;
static volatile bool readChargerNow = false;

static uint16_t minutesUntilAutoOff;
static uint16_t minutesUntilLockAfterAutoOff;
static volatile uint32_t ticksSinceLastUserActivity = 0;

static CliInput cliInput;
static RGBLed *caseLed;
static BQ25180 *chargerIC;
static MC3479 *accel;
static WriteToUsbSerial *writeUsbSerial;
static void (*enterDFU)();
static uint8_t (*readButtonPin)();
static void (*writeBulbLedPin)(uint8_t state);
static float (*getMillisecondsPerChipTick)();
static void (*startLedTimers)(); // TODO: split bulb timer and rgb timer control into
static void (*stopLedTimers)();  //       different functions. Move rgb timers to RGB

static const char *fakeOffMode = "{\"command\":\"writeMode\",\"index\":255,\"mode\":{\"name\":\"fakeOff\",\"front\":{\"pattern\":{\"type\":\"simple\",\"name\":\"off\",\"duration\":100,\"changeAt\":[{\"ms\":0,\"output\":\"low\"}]}}}}";
static const char *defaultMode = "{\"command\":\"writeMode\",\"index\":0,\"mode\":{\"name\":\"full on\",\"front\":{\"pattern\":{\"type\":\"simple\",\"name\":\"on\",\"duration\":100,\"changeAt\":[{\"ms\":0,\"output\":\"high\"}]}}}}";

static void readBulbModeWithBuffer(uint8_t modeIndex, char *buffer) {
	readBulbModeFromFlash(modeIndex, buffer, 1024);
	parseJson(buffer, 1024, &cliInput);

	if (cliInput.parsedType != parseWriteMode) {
		// fallback to default
		parseJson(defaultMode, 1024, &cliInput);
	}
}

static void readBulbMode(uint8_t modeIndex) {
	if (modeIndex == fakeOffModeIndex) {
		parseJson(fakeOffMode, 1024, &cliInput);
	} else {
		char flashReadBuffer[1024];
		readBulbModeWithBuffer(modeIndex, flashReadBuffer);
	}
}

static void setCurrentMode(Mode *mode, uint8_t index) {
	currentMode = *mode;
	currentModeIndex = index;

	if (currentModeIndex == fakeOffModeIndex) {
		stopLedTimers();
	} else {
		startLedTimers();
	}

	if (currentMode.has_accel && currentMode.accel.triggers_count > 0) {
		mc3479Enable(accel);
	} else {
		mc3479Disable(accel);
	}
}

static void setCurrentModeIndex(uint8_t modeIndex) {
	readBulbMode(modeIndex);
	setCurrentMode(&cliInput.mode, modeIndex);
}

static void readSettingsWithBuffer(ChipSettings *settings, char *buffer) {
	// set some defaults
	settings->modeCount = 0;
	settings->minutesUntilAutoOff = 90;
	settings->minutesUntilLockAfterAutoOff = 10;

	readSettingsFromFlash(buffer, 1024);
	parseJson(buffer, 1024, &cliInput);

	if (cliInput.parsedType == parseWriteSettings) {
		*settings = cliInput.settings;
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
		RGBLed *_caseLed,
		WriteToUsbSerial *_writeUsbSerial,
		void (*_enterDFU)(),
		uint8_t (*_readButtonPin)(),
		void (*_writeBulbLedPin)(uint8_t state),
		float (*_getMillisecondsPerChipTick)(),
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
	getMillisecondsPerChipTick = _getMillisecondsPerChipTick;
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

static void buttonInputTask(uint16_t tick, float millisPerTick) {
	static int16_t clickStartTick = 0;
	
	if (hasClickStarted() && clickStartTick == 0) {
		clickStartTick = tick;
	}

	uint16_t elapsedMillis = 0;
	if (clickStartTick != 0) {
		uint16_t elapsedTicks = tick - clickStartTick;
		elapsedMillis = elapsedTicks * millisPerTick;
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
		case ignore:
			break;
		case clicked:
			rgbShowSuccess(caseLed);
			uint8_t newModeIndex = currentModeIndex + 1;
			if (newModeIndex >= modeCount) {
				newModeIndex = 0;
			}
			setCurrentModeIndex(newModeIndex);
			const char *blah = "clicked\n";
			writeUsbSerial(0, blah, strlen(blah));
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
	if (hasClickStarted() || currentModeIndex != fakeOffModeIndex) {
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

static void chargerTask(uint16_t tick, float millisPerTick) {
	static enum ChargeState chargingState = notConnected;
	static uint16_t checkedAtTick = 0;

	uint8_t previousState = chargingState;
	uint16_t elapsedMillis = 0;

	if (checkedAtTick != 0) {
		uint16_t elapsedTicks = tick - checkedAtTick;
		elapsedMillis = elapsedTicks * millisPerTick;
	}

	// charger i2c watchdog timer will reset if not communicated
	// with for 40 seconds, and 15 seconds after plugged in.
	if (elapsedMillis > 30000 || checkedAtTick == 0) {
		// char registerJson[256];
		// readAllRegistersJson(chargerIC, registerJson);
		// writeUsbSerial(0, registerJson, strlen(registerJson));
		// printAllRegisters(chargerIC);

		chargingState = getChargingState(chargerIC);
		checkedAtTick = tick;
	}

	// flash charging state to user every second
	if (chargingState != notConnected && elapsedMillis % 1000 >= 1000 - millisPerTick) {
		showChargingState(chargingState);
	}

	if (readChargerNow) {
		readChargerNow = false;
		enum ChargeState state = getChargingState(chargerIC);

		bool wasDisconnected = previousState != notConnected && state == notConnected;
		if (tick != 0 && wasDisconnected && currentModeIndex == fakeOffModeIndex) {
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
	float millisPerTick = getMillisecondsPerChipTick();
	buttonInputTask(chipTick, millisPerTick);
	rgbTask(caseLed, chipTick, millisPerTick);
	mc3479Task(accel, chipTick, millisPerTick);
	chargerTask(chipTick, millisPerTick);
}

void handleChargerInterrupt() {
	readChargerNow = 1;
}

//// Helper to get output from a simple pattern at a specific time
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

static void updateMode() {
	static uint16_t modeMs = 0;
	
	modeMs += getMillisecondsPerChipTick();

	// Check triggers
	ModeComponent frontComp = currentMode.front;
	ModeComponent caseComp = currentMode.case_comp;
	bool triggered = false;

	if (currentMode.has_accel && currentMode.accel.triggers_count > 0) {
		// TODO: check for configurable trigger threshold
		// For now using hardcoded 0.3f, but should use trigger.threshold
		if (isOverThreshold(accel, 0.3f)) {
			// Use first trigger for now
			ModeAccelTrigger trigger = currentMode.accel.triggers[0];

			// TODO: indicate fallthrough from mode if not specified in UI
			if (trigger.has_front) frontComp = trigger.front;
			if (trigger.has_case_comp) caseComp = trigger.case_comp;
			triggered = true;
		}
	}

	// Update Front (Bulb and RGB)
	if (currentMode.has_front || (triggered && currentMode.accel.triggers[0].has_front)) {
		if (frontComp.pattern.type == PATTERN_TYPE_SIMPLE && frontComp.pattern.data.simple.changeAt_count > 0) {
			SimpleOutput output = getOutputFromSimplePattern(&frontComp.pattern.data.simple, (uint32_t)modeMs);
			if (output.type == BULB) {
				if (output.data.bulb == high) {
					writeBulbLedPin(1);
				} else {
					writeBulbLedPin(0);
				}
			} else {
				// TOD0: RGB
				writeBulbLedPin(0);
			}
		}
	} else {
		writeBulbLedPin(0);
	}

	// Update Case (RGB only)
	if (!hasClickStarted()) {
		if (currentMode.has_case_comp || (triggered && currentMode.accel.triggers[0].has_case_comp)) {
			if (caseComp.pattern.type ==  PATTERN_TYPE_SIMPLE && caseComp.pattern.data.simple.changeAt_count > 0) {
				SimpleOutput output = getOutputFromSimplePattern(&caseComp.pattern.data.simple, (uint32_t)modeMs);
				if (output.type == RGB) {
					rgbShowUserColor(caseLed, output.data.rgb.r, output.data.rgb.g, output.data.rgb.b);
				} else {
					// TODO: BULB
				}
			}
		} else {
			// Default to off if no case component
			rgbShowUserColor(caseLed, 0, 0, 0);
		}
	}
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
		if (currentModeIndex == fakeOffModeIndex) {
			uint16_t ticksUntilLockAfterAutoOff = minutesUntilLockAfterAutoOff * 60 / 10;
			autoOffTimerDone = ticksSinceLastUserActivity > ticksUntilLockAfterAutoOff;
		}

		if (autoOffTimerDone) {
			if (currentModeIndex == fakeOffModeIndex) {
				lock();
			} else {
				ticksSinceLastUserActivity = 0; // restart timer to transition from fakeOff to shipMode
				shutdownFake();
			}
		}
	}
}

void handleJson(uint8_t buf[], uint32_t count) {
	parseJson(buf, count, &cliInput);

	switch (cliInput.parsedType) {
	case parseError: {
		// TODO return errors from mode parser
		char error[] = "{\"error\":\"unable to parse json\"}\n";
		writeUsbSerial(0, error, strlen(error));
		break;
	}
	case parseWriteMode: {
		if (strcmp(cliInput.mode.name, "transientTest") == 0) {
			// do not write to flash for transient test
			setCurrentMode(&cliInput.mode, cliInput.modeIndex);
		} else {
			writeBulbModeToFlash(cliInput.modeIndex, buf, cliInput.jsonLength);
			setCurrentMode(&cliInput.mode, cliInput.modeIndex);
		}
		break;
	}
	case parseReadMode: {
		char flashReadBuffer[1024];
		readBulbModeWithBuffer(cliInput.modeIndex, flashReadBuffer);
		uint16_t len = strlen(flashReadBuffer);
		flashReadBuffer[len] = '\n';
		flashReadBuffer[len + 1] = '\0';
		writeUsbSerial(0, flashReadBuffer, strlen(flashReadBuffer));
		break;
	}
	case parseWriteSettings: {
		ChipSettings settings = cliInput.settings;
		writeSettingsToFlash(buf, cliInput.jsonLength);
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

	if (cliInput.parsedType != parseError) {
		rgbShowSuccess(caseLed);
	}
}
