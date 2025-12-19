/*
 * button.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "device/button.h"

static volatile bool processButtonInterrupt = false;
void startButtonEvaluation() {
	processButtonInterrupt = true;
}

bool buttonInit(
		Button *button,
		uint8_t (*readButtonPin)(),
		void (*startButtonTimer)(),
		void (*stopButtonTimer)(),
		RGBLed *caseLed
) {
	if (!button || !caseLed) {
		return false;
	}

	button->readButtonPin = readButtonPin;
	button->startButtonTimer = startButtonTimer;
	button->stopButtonTimer = stopButtonTimer;
	button->caseLed = caseLed;

	button->evalStartTick = 0;
	return true;
}

enum ButtonResult buttonInputTask(Button *button, uint16_t tick, float millisPerTick) {
	if (processButtonInterrupt && button->evalStartTick == 0) {
		button->evalStartTick = tick;
		rgbShowNoColor(button->caseLed);
		// Timers interrupts needed to properly detect input
		button->startButtonTimer();
	}

	uint16_t elapsedMillis = 0;
	if (button->evalStartTick != 0) {
		uint16_t elapsedTicks = tick - button->evalStartTick;
		elapsedMillis = elapsedTicks * millisPerTick;
	}

	uint8_t state = button->readButtonPin();
	bool buttonCurrentlyDown = state == 0;

	if (buttonCurrentlyDown) {
		if (elapsedMillis > 2000 && elapsedMillis < 2100) {
			rgbShowLocked(button->caseLed);
		} else if (elapsedMillis > 1000 && elapsedMillis < 1100) {
			rgbShowShutdown(button->caseLed);
		}
	}

	if (!buttonCurrentlyDown && elapsedMillis > 20) {
		enum ButtonResult buttonState = clicked;
		if (elapsedMillis > 2000) {
			buttonState = lockOrHardwareReset;
		} else if (elapsedMillis > 1000) {
			buttonState = shutdown;
		} else {
			buttonState = ignore;
		}

		if (buttonState == clicked) {
			rgbShowSuccess(button->caseLed);
		}

		button->evalStartTick = 0;
		processButtonInterrupt = false;
//		ticksSinceLastUserActivity = 0; // TODO: autoOff, refactor to do this in chipState
		return buttonState;
	}
	return ignore;
}

bool isEvaluatingButtonPress(Button *button) {
	// could also just check processButtonInterrupt == true, but button eval is first task after interrupt, so evalStartTick is fine
	return button->evalStartTick != 0;
}
