/*
 * button.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "microlight/device/button.h"

bool buttonInit(
    Button *button,
    uint8_t (*readButtonPin)(),
    void (*enableChipTickTimer)(bool enable),
    RGBLed *caseLed) {
    if (!button || !caseLed || !readButtonPin || !enableChipTickTimer) {
        return false;
    }

    button->readButtonPin = readButtonPin;
    button->enableChipTickTimer = enableChipTickTimer;
    button->caseLed = caseLed;

    button->evalStartMs = 0;
    return true;
}

enum ButtonResult buttonInputTask(Button *button, uint32_t milliseconds, bool interruptTriggered) {
    if (interruptTriggered && button->evalStartMs == 0) {
        button->evalStartMs = milliseconds;
        rgbShowNoColor(button->caseLed);
        // Timers interrupts needed to properly detect input
        button->enableChipTickTimer(true);
    }

    uint32_t elapsedMillis = 0;
    if (button->evalStartMs != 0) {
        elapsedMillis = milliseconds - button->evalStartMs;
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

    if (!buttonCurrentlyDown && elapsedMillis > 50) {
        enum ButtonResult buttonState = clicked;
        if (elapsedMillis > 2000) {
            buttonState = lockOrHardwareReset;
        } else if (elapsedMillis > 1000) {
            buttonState = shutdown;
        }

        if (buttonState == clicked) {
            rgbShowSuccess(button->caseLed);
        }

        button->evalStartMs = 0;
        return buttonState;
    }
    return ignore;
}

bool isEvaluatingButtonPress(Button *button) {
    return button->evalStartMs != 0;
}
