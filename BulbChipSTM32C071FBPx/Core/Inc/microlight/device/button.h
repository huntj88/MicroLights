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

#include "microlight/device/rgb_led.h"

typedef struct Button {
    uint8_t (*readButtonPin)();
    void (*enableChipTickTimer)(bool enable);
    RGBLed *caseLed;

    uint32_t evalStartMs;
} Button;

bool buttonInit(
    Button *button,
    uint8_t (*readButtonPin)(),
    void (*enableChipTickTimer)(bool enable),
    RGBLed *caseLed);

enum ButtonResult {
    ignore,
    clicked,
    shutdown,
    lockOrHardwareReset  // hardware reset occurs when usb is plugged in
};

enum ButtonResult buttonInputTask(Button *button, uint32_t milliseconds, bool interruptTriggered);
bool isEvaluatingButtonPress(Button *button);

#endif /* INC_DEVICE_BUTTON_H_ */
