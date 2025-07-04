/*
 * rgb.c
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#include <stdint.h>
#include "stm32c0xx_hal.h"
#include "rgb.h"

static uint8_t loopCount = 0;
static uint8_t tickOfNotification = 0;
static uint8_t showingNotification = 0;

void rgb_task() {
	loopCount++;

	if (showingNotification && tickOfNotification + 40 == loopCount) {
		showingNotification = 0;
		showColor(0, 0, 0);
	}
}

void showFailure() {
	tickOfNotification = loopCount;
	showingNotification = 1;
	showColor(20, 0, 0);
}

void showSuccess() {
	tickOfNotification = loopCount;
	showingNotification = 1;
	showColor(0, 20, 0);
}

void showLocked() {
	tickOfNotification = loopCount;
	showingNotification = 1;
	showColor(0, 0, 20);
}

// expect red, green, blue to be in range of 0 to 255
void showColor(uint8_t red, uint8_t green, uint8_t blue) {
	uint16_t max = 30000; // 100% duty cycle is actually 47999 (check ioc file), but limiting for now
	uint16_t increment = max / 256;

	TIM1->CCR1 = blue * increment;
	TIM1->CCR2 = green * increment;
	TIM1->CCR3 = red * increment;
}
