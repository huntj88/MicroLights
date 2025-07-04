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
static uint8_t buttonState = 0;
void handleButtonInput(void (*shutdown)()) {
	if (hasClickStarted()) {
		GPIO_PinState state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_5);
		uint8_t buttonCurrentlyDown = state == GPIO_PIN_RESET;

		if (buttonCurrentlyDown) {
			buttonDownCounter += 1;
			if (buttonDownCounter > 200 && buttonState == 0) {
				showColor(0, 0, 0);
				buttonState = 1; // shutdown
			} else if (buttonDownCounter > 1200 && buttonState == 1) {
				showLocked();
//				buttonState = 2; // shutdown and lock
			}
		} else {
			buttonDownCounter -= 10; // Large decrement to allow any hold time to "discharge" quickly.
			if (buttonDownCounter < -200) {
				buttonDownCounter = 0;
				if (buttonState == 0) {
					// Button clicked and released.
					setClickEnded();

					uint8_t newModeIndex = currentMode.modeIndex + 1;
					if (newModeIndex > 2) { // TODO: config json to track settings, like how many modes exist?
						newModeIndex = 0;
					}
					BulbMode newMode = readBulbMode(newModeIndex);
					setCurrentMode(newMode);
				} else if (buttonState == 1) {
					shutdown();
				} else if (buttonState == 2) {
					// TODO: Lock
				}
			}
		}
	}
}
