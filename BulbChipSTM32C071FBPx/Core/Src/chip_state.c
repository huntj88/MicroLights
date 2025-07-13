/*
 * chip_state.c
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#include <stdint.h>
#include "stm32c0xx_hal.h"
#include "chip_state.h"
#include "storage.h"
#include "rgb.h"

// TODO: don't read flash every time mode changes?, can be cached
static volatile uint8_t modeCount = 0;
static volatile BulbMode currentMode;
static volatile uint8_t clickStarted = 0;
static volatile uint8_t readChargerNow = 0;

static BQ25180 *chargerIC;
static WriteToUsbSerial *writeUsbSerial;
static void (*shutdown)();

static void readBulbMode(uint8_t modeIndex, BulbMode *mode) {
	CliInput input;
	char flashReadBuffer[1024];
	readBulbModeFromFlash(modeIndex, flashReadBuffer, 1024);
	parseJson(flashReadBuffer, 1024, &input);

	if (input.mode.numChanges > 0 && input.mode.totalTicks > 0) {
		// successfully read from flash
		*mode = input.mode;
	} else {
		// fallback to default
		char * defaultMode = "{\"command\":\"setMode\",\"index\":0,\"mode\":{\"name\":\"default\",\"totalTicks\":1,\"changeAt\":[{\"tick\":0,\"output\":\"high\"}]}}";
		parseJson(defaultMode, 1024, &input);
		*mode = input.mode;
	}
}

static void readSettings(ChipSettings *settings) {
	CliInput input;
	char flashReadBuffer[1024];
	readSettingsFromFlash(flashReadBuffer, 1024);
	parseJson(flashReadBuffer, 1024, &input);
	*settings = input.settings;
}

void configureChipState(BQ25180 *_chargerIC, WriteToUsbSerial *_writeUsbSerial, void (*_shutdown)()) {
	chargerIC = _chargerIC;
	shutdown = _shutdown;
	writeUsbSerial = _writeUsbSerial;

	BulbMode mode;
	readBulbMode(0, &mode);
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
	char *blah = "clicked\n";
	writeUsbSerial(0, blah, strlen(blah));
}

static uint8_t hasClickStarted() {
	return clickStarted;
}

static void showChargingState() {
	uint8_t state = getChargingState(chargerIC);
	if (state == NOT_CONNECTED) {
		showColor(0, 0, 0);
	} else if (state == NOT_CHARGING) {
		showColor(40, 0, 0);
	} else if (state == CONSTANT_CURRENT_CHARGING) {
		showColor(30, 10, 0);
	} else if (state == CONSTANT_VOLTAGE_CHARGING) {
		showColor(10, 30, 0);
	} else if (state == DONE_CHARGING) {
		showColor(0, 40, 0);
	}
}

static void handleButtonInput(void (*shutdown)()) {
	// Button states
	// 0: confirming click
	// 1: clicked
	// 2: shutdown
	// 3: lock and shutdown
	// 4: lock cancelled, shutdown
	static uint8_t buttonState = 0;
	static int16_t buttonDownCounter = 0;

	if (hasClickStarted()) {
		GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
		uint8_t buttonCurrentlyDown = state == GPIO_PIN_RESET;

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
			} else if (buttonDownCounter > 1000 && buttonState == 3) {
				buttonState = 4; // lock cancelled, shut down
				showNoColor();
			}

			// prevent long holds from increasing time for button action to start
			if (buttonDownCounter > 1000) {
				buttonDownCounter = 1000;
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
					shutdown();
				} else if (buttonState == 3) {
					// TODO: lock
					shutdown();
				}
				setClickEnded();
				buttonState = 0;
			}
		}
	}
}

void stateTask() {
	static uint16_t tickCount = 0;

	if (tickCount % 1024 == 0) {
		configureChargerIC(chargerIC);
		printAllRegisters(chargerIC);
		showChargingState(chargerIC);
	}

	if (readChargerNow) {
		readChargerNow = 0;
		showChargingState(chargerIC);
//		printAllRegisters(chargerIC);
	}

	handleButtonInput(shutdown);

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
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
		} else {
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
		}

		if (currentChangeIndex + 1 < currentMode.numChanges) {
			nextTickInMode = currentMode.changeAt[currentChangeIndex + 1].tick;
		}

		currentChangeIndex++;
	}

	modeInterruptCount++;
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
	}
}
