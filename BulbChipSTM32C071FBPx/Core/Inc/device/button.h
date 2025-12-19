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

#include "device/rgb_led.h"

// called from button interrupt
void startButtonEvaluation();

typedef struct Button {
	uint8_t (*readButtonPin)();
	void (*startButtonTimer)();
	void (*stopButtonTimer)();

	bool isEvaluating;
	uint16_t tick;
	uint16_t evalStartTick;

	RGBLed *caseLed;
} Button;

bool buttonInit(
	Button *button,
	uint8_t (*readButtonPin)(),
	void (*startButtonTimer)(),
	void (*stopButtonTimer)(),
	RGBLed *caseLed
);

enum ButtonResult {
	ignore,
	clicked,
	shutdown,
	lockOrHardwareReset // hardware reset occurs when usb is plugged in
};

enum ButtonResult buttonInputTask(Button *button, uint16_t tick, float millisPerTick);

#endif /* INC_DEVICE_BUTTON_H_ */
