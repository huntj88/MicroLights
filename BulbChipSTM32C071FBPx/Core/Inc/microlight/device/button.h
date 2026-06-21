/*
 * button.h
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#ifndef INC_DEVICE_BUTTON_H_
#define INC_DEVICE_BUTTON_H_

#include <stdbool.h>
#include <stdint.h>

typedef struct Button {
    uint8_t (*readButtonPin)();

    uint32_t evalStartMs;
} Button;

bool buttonInit(Button *button, uint8_t (*readButtonPin)());

enum ButtonResult {
    ignore,
    clicked,
    indicateShutdown,             // IndicateShutdown is for status LED effects.
    shutdown,                     // Activates when button released
    indicateLockOrHardwareReset,  // IndicateLockOrHardwareReset is for status LED effects.
    lockOrHardwareReset,  // Activates when button released. Hardware reset occurs when usb is
                          // plugged in.
};

enum ButtonResult buttonInputTask(Button *button, uint32_t milliseconds, bool interruptTriggered);
bool isEvaluatingButtonPress(Button *button);

#endif /* INC_DEVICE_BUTTON_H_ */
