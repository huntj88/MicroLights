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
static uint8_t showingTransientStatus = 0;
static uint8_t tickCountToShowTransient = 75;

// expect red, green, blue to be in range of 0 to 255
static void showColor(uint8_t red, uint8_t green, uint8_t blue) {
	uint16_t max = 30000; // 100% duty cycle is actually 47999 (check ioc file), but limiting for now
	uint16_t increment = max / 256;

	TIM1->CCR1 = blue * increment;
	TIM1->CCR2 = green * increment;
	TIM1->CCR3 = red * increment;

	tickOfStatusUpdate = tickCount;
}

void rgb_task() {
	tickCount++;

	// show status color for 3 ticks
	if (showingTransientStatus && tickOfStatusUpdate + 75 == tickCount) {
		showingTransientStatus = 0;
		showNoColor();
	}
}

void showSuccess() {
	showingTransientStatus = 1;
	showColor(10, 10, 10);
}

void showFailure() {
	showColor(10, 0, 10);
}

void showLocked() {
	showColor(0, 0, 20);
}

void showShutdown() {;
	showColor(20, 20, 20);
}

void showNoColor() {
	showColor(0, 0, 0);
}

void showNotCharging() {
	showingTransientStatus = 1;
	showFailure();
}

void showConstantCurrentCharging() {
	showingTransientStatus = 1;
	showColor(20, 0, 0);
}

void showConstantVoltageCharging() {
	showingTransientStatus = 1;
	showColor(10, 10, 0);
}

void showDoneCharging() {
	showingTransientStatus = 1;
	showColor(0, 10, 0);
}
