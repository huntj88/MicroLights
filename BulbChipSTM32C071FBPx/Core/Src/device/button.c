/*
 * button.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "device/button.h"

// more than one button not likely,
// if handling multiple buttons would need to pass in function pointer that get bool for specific
// interrupt variable
static volatile bool processButtonInterrupt = false;

void startButtonEvaluation() {
    processButtonInterrupt = true;
}

bool buttonInit(Button *button,
                uint8_t (*readButtonPin)(),
                void (*startButtonTimer)(),
                void (*stopButtonTimer)(),
                RGBLed *caseLed) {
    if (!button || !caseLed) {
        return false;
    }

    button->readButtonPin = readButtonPin;
    button->startButtonTimer = startButtonTimer;
    button->stopButtonTimer = stopButtonTimer;
    button->caseLed = caseLed;

    button->evalStartMs = 0;
    return true;
}

enum ButtonResult buttonInputTask(Button *button, uint32_t ms) {
    // shadow evalStartMs to avoid multiple reads of volatile
    if (processButtonInterrupt && button->evalStartMs == 0) {
        button->evalStartMs = ms;
        rgbShowNoColor(button->caseLed);
        // Timers interrupts needed to properly detect input
        button->startButtonTimer();
    }

    uint32_t elapsedMillis = 0;
    if (button->evalStartMs != 0) {
        elapsedMillis = ms - button->evalStartMs;
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
        }

        if (buttonState == clicked) {
            rgbShowSuccess(button->caseLed);
        }

        button->evalStartMs = 0;
        processButtonInterrupt = false;
        return buttonState;
    }
    return ignore;
}

bool isEvaluatingButtonPress(Button *button) {
    // coupled to interrupt variable, see comment on variable
    return processButtonInterrupt;
}
