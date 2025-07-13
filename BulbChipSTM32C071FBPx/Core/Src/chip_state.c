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
#include "tusb.h"

// TODO: don't read flash every time mode changes?, can be cached
static volatile BulbMode currentMode;
static volatile uint8_t clickStarted = 0;
static char flashReadBuffer[1024];

void readBulbMode(uint8_t modeIndex, BulbMode *mode) {
	readBulbModeFromFlash(modeIndex, flashReadBuffer, 1024);
	parseJson(flashReadBuffer, 1024, mode);
}

void setInitialState() {
	BulbMode mode;
	readBulbMode(0, &mode);

//	char json[] = "{\"name\":\"blah0\",\"modeIndex\":0,\"totalTicks\":20,\"changeAt\":[{\"tick\":0,\"output\":\"high\"},{\"tick\":1,\"output\":\"low\"}]}";
//	BulbMode mode = parseJson(json, 1024);
	currentMode = mode;
}

void setClickStarted() {
	clickStarted = 1;
}

void setClickEnded() {
	clickStarted = 0;
}

uint8_t hasClickStarted() {
	return clickStarted;
}

void handleButtonInput(void (*shutdown)()) {
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
			buttonDownCounter -= 25; // Large decrement to allow any hold time to "discharge" quickly.
			if (buttonDownCounter < -50) {
				buttonDownCounter = 0;
				if (buttonState == 0) {
					// ignore, probably pulse from chargerIC?
				} else if (buttonState == 1) {
					// Button clicked and released.
					showSuccess();
					uint8_t newModeIndex = currentMode.modeIndex + 1;
					if (newModeIndex > 1) { // TODO: config json to track settings, like how many modes exist?
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

void cdc_task() {
	static uint8_t jsonBuf[1024];
	static uint16_t jsonIndex = 0;
	uint8_t itf;

	for (itf = 0; itf < CFG_TUD_CDC; itf++) {
		// connected() check for DTR bit
		// Most but not all terminal client set this when making connection
		// if ( tud_cdc_n_connected(itf) )
		{
			if (tud_cdc_n_available(itf)) {
				uint8_t buf[64];
				uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));
				for (uint8_t i = 0; i < count; i++) {
					jsonBuf[jsonIndex + i] = buf[i];
				}
				jsonIndex += count;
			} else if (jsonIndex != 0) {
				jsonIndex = 0;
				BulbMode mode;
				parseJson(jsonBuf, 1024, &mode);

				if (mode.numChanges > 0 && mode.totalTicks > 0) {
					writeBulbModeToFlash(mode.modeIndex, jsonBuf, mode.jsonLength);
					currentMode = mode;
					showSuccess();
				}
			}
		}
	}
}
