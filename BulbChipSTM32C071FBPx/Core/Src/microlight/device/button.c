/*
 * button.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "microlight/device/button.h"

const uint32_t shutdownHoldMillis = 500;
const uint32_t lockHoldMillis = 1500;

bool buttonInit(Button *button, uint8_t (*readButtonPin)()) {
    if (!button || !readButtonPin) {
        return false;
    }

    button->readButtonPin = readButtonPin;

    button->evalStartMs = 0;
    return true;
}

enum ButtonResult buttonInputTask(Button *button, uint32_t milliseconds, bool interruptTriggered) {
    uint8_t state = button->readButtonPin();
    bool buttonCurrentlyDown = state == 0;

    if (interruptTriggered && button->evalStartMs == 0 && buttonCurrentlyDown) {
        button->evalStartMs = milliseconds;
        // See timer policy in chip_state.
        // ChipTick timer needed to properly detect input. Main loop needs to run to detect
        // that button is being held and show appropriate status.
    }

    uint32_t elapsedMillis = 0;
    if (button->evalStartMs != 0) {
        elapsedMillis = milliseconds - button->evalStartMs;
    }

    enum ButtonResult buttonState = ignore;

    if (buttonCurrentlyDown) {
        if (elapsedMillis > lockHoldMillis) {
            buttonState = indicateLockOrHardwareReset;
        } else if (elapsedMillis > shutdownHoldMillis) {
            buttonState = indicateShutdown;
        }
    }

    if (!buttonCurrentlyDown && button->evalStartMs != 0) {
        if (elapsedMillis <= 50) {
            button->evalStartMs = 0;
            return ignore;
        }

        buttonState = clicked;
        if (elapsedMillis > lockHoldMillis) {
            buttonState = lockOrHardwareReset;
        } else if (elapsedMillis > shutdownHoldMillis) {
            buttonState = shutdown;
        }

        button->evalStartMs = 0;
    }
    return buttonState;
}

bool isEvaluatingButtonPress(Button *button) {
    return button->evalStartMs != 0;
}
