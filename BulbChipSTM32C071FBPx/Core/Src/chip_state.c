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
	// TODO
//	BulbMode mode = readBulbMode(0);

	char json[] = "{\"name\":\"blah0\",\"modeIndex\":0,\"totalTicks\":20,\"changeAt\":[{\"tick\":0,\"output\":\"high\"},{\"tick\":1,\"output\":\"low\"}]}";
	BulbMode mode = parseJson(json, 1024);
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
		GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
		uint8_t buttonCurrentlyDown = state == GPIO_PIN_RESET;

		if (buttonCurrentlyDown) {
			buttonDownCounter += 1;
			if (buttonDownCounter > 200 && buttonState == 0) {
				showShutdown();
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
					showSuccess(); // debugging
					// Button clicked and released.


//					uint8_t newModeIndex = currentMode.modeIndex + 1;
//					if (newModeIndex > 2) { // TODO: config json to track settings, like how many modes exist?
//						newModeIndex = 0;
//					}
//					BulbMode newMode = readBulbMode(newModeIndex);
//					setCurrentMode(newMode);
				} else if (buttonState == 1) {
					showShutdown();
					shutdown();
				} else if (buttonState == 2) {
					// TODO: Lock
				}

				setClickEnded();
				buttonState = 0;
			}
		}
	}
}
