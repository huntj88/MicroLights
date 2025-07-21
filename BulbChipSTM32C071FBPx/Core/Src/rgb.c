/*
 * rgb.c
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#include <stdint.h>
#include <stdbool.h>
#include "stm32c0xx_hal.h"
#include "rgb.h"

static uint16_t tickCount = 0;
static uint16_t tickOfStatusUpdate = 0;
static bool showingTransientStatus = false;

// expect red, green, blue to be in range of 0 to 255
static void showColor(uint8_t red, uint8_t green, uint8_t blue) {
	uint16_t max = 30000; // 100% duty cycle is actually 47999 (check ioc file), but limiting for now
	uint16_t increment = max / 256;

	TIM1->CCR1 = red * increment;
	TIM1->CCR2 = green * increment;
	TIM1->CCR3 = blue * increment;

	tickOfStatusUpdate = tickCount;
}

void rgb_task() {
	tickCount++;

	// show status color for 3 ticks
	if (showingTransientStatus && tickOfStatusUpdate + 75 == tickCount) {
		showingTransientStatus = false;
		showNoColor();
	}
}

void showSuccess() {
	showingTransientStatus = true;
	showColor(10, 10, 10);
}

void showFailure() {
	showColor(10, 0, 10);
}

void showLocked() {
	showColor(0, 0, 20);
}

void showShutdown() {
	showingTransientStatus = true;
	showColor(20, 20, 20);
}

void showNoColor() {
	showColor(0, 0, 0);
}

void showNotCharging() {
	showingTransientStatus = true;
	showFailure();
}

void showConstantCurrentCharging() {
	showingTransientStatus = true;
	showColor(20, 0, 0);
}

void showConstantVoltageCharging() {
	showingTransientStatus = true;
	showColor(10, 10, 0);
}

void showDoneCharging() {
	showingTransientStatus = true;
	showColor(0, 10, 0);
}
