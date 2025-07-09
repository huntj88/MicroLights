/*
 * rgb.c
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#include <stdint.h>
#include "stm32c0xx_hal.h"
#include "rgb.h"

static uint8_t tickCount = 0;
static uint8_t tickOfStatusUpdate = 0;
static uint8_t showingStatus = 0;

void rgb_task() {
	tickCount++;

	// show status color for 40 ticks
	if (showingStatus && tickOfStatusUpdate + 40 == tickCount) {
		showingStatus = 0;
		showColor(0, 0, 0);
	}
}

void showFailure() {
	tickOfStatusUpdate = tickCount;
	showingStatus = 1;
	showColor(20, 0, 0);
}

void showSuccess() {
	tickOfStatusUpdate = tickCount;
	showingStatus = 1;
	showColor(0, 20, 0);
}

void showLocked() {
	tickOfStatusUpdate = tickCount;
	showingStatus = 1;
	showColor(0, 0, 20);
}

void showShutdown() {
	tickOfStatusUpdate = tickCount;
	showingStatus = 1;
	showColor(20, 20, 20);
}

// expect red, green, blue to be in range of 0 to 255
void showColor(uint8_t red, uint8_t green, uint8_t blue) {
	uint16_t max = 30000; // 100% duty cycle is actually 47999 (check ioc file), but limiting for now
	uint16_t increment = max / 256;

	TIM1->CCR1 = blue * increment;
	TIM1->CCR2 = green * increment;
	TIM1->CCR3 = red * increment;
}
