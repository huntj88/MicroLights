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

// TODO: don't read flash every time mode changes?, can be cached
static volatile BulbMode currentMode;
static volatile uint8_t clickStarted = 0;
static char flashReadBuffer[1024];

BulbMode readBulbMode(uint8_t modeIndex) {
	readBulbModeFromFlash(modeIndex, flashReadBuffer, 1024);
	BulbMode mode = parseJson(flashReadBuffer, 1024);
	return mode;
}

void setInitialState() {
	BulbMode mode = readBulbMode(0);
	setCurrentMode(mode);
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

void setCurrentMode(BulbMode mode) {
	currentMode = mode;
}

BulbMode getCurrentMode() {
	return currentMode;
}

static int16_t buttonDownCounter = 0;
void handleButtonInput() {
	if (hasClickStarted()) {
		// TODO: Disable button interrupt when button is clicked, until click is over, or until shutdown? not sure if necessary

		GPIO_PinState state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);
		uint8_t buttonCurrentlyDown = state == GPIO_PIN_RESET;

		if (buttonCurrentlyDown) {
			buttonDownCounter += 1;
			if (buttonDownCounter > 200) {
				// TODO: Hold button down to shut down.
			}
		} else {
			buttonDownCounter -= 10; // Large decrement to allow any hold time to "discharge" quickly.
			if (buttonDownCounter < -200) {
				// Button clicked and released.
				setClickEnded();
				buttonDownCounter = 0;

				uint8_t newModeIndex = currentMode.modeIndex + 1;
				if (newModeIndex > 2) { // TODO: config json to track settings, like how many modes exist?
					newModeIndex = 0;
				}
				BulbMode newMode = readBulbMode(newModeIndex);
				setCurrentMode(newMode);
			}
		}
	}
}
