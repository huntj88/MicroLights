/*
 * rgb.c
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#include <stdint.h>
#include <stdbool.h>
#include "stm32c0xx_hal.h" // TODO: pass in register changes as param? treat like accelerometer or charger implementations
#include "rgb.h"

static uint16_t tickCount = 0;
static uint16_t tickOfStatusUpdate = 0;
static bool showingTransientStatus = false;
static uint8_t rUser, gUser, bUser;

// TODO: multiple priorities, could create a prioritized led resource mutex if it gets more complicated
// button input
// user defined mode color
// charging

// expect red, green, blue to be in range of 0 to 255
static void showColor(uint8_t red, uint8_t green, uint8_t blue) { 
	uint16_t max = 4000; // 100% duty cycle is actually 47999 (check ioc file), but limiting for now
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
		showColor(rUser, gUser, bUser);
	}
}

void showUserColor(uint8_t red, uint8_t green, uint8_t blue) {
	rUser = red;
	gUser = green;
	bUser = blue;

	if (!showingTransientStatus) {
		showColor(red, green, blue);
	}
}

void showSuccess() {
	showingTransientStatus = true;
	showColor(40, 40, 40);
}

void showFailure() {
	showColor(40, 0, 40);
}

void showLocked() {
	showColor(0, 0, 80);
}

void showShutdown() {
	showingTransientStatus = true;
	showColor(80, 80, 80);
}

void showNoColor() {
	showUserColor(0, 0, 0);
}

void showNotCharging() {
	showingTransientStatus = true;
	showFailure();
}

void showConstantCurrentCharging() {
	showingTransientStatus = true;
	showColor(80, 0, 0);
}

void showConstantVoltageCharging() {
	showingTransientStatus = true;
	showColor(40, 40, 0);
}

void showDoneCharging() {
	showingTransientStatus = true;
	showColor(0, 40, 0);
}
