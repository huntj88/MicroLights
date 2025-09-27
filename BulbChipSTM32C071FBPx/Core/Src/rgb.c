/*
 * rgb.c
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#include <stddef.h>
#include "rgb.h"

// TODO: multiple priorities, could create a prioritized led resource mutex if it gets more complicated
// button input
// user defined mode color
// charging

static uint16_t colorRangeToDuty(const RGB *device, uint8_t value) {
	uint16_t increment = device->period / 255U;
	return value * increment;
}

// expect red, green, blue to be in range of 0 to 255
static void showColor(RGB *device, uint8_t red, uint8_t green, uint8_t blue, bool transient) {
	if (!device || !device->writePwm) {
		return;
	}

	device->showingTransientStatus = transient;

	uint16_t scaledRed = colorRangeToDuty(device, red);
	uint16_t scaledGreen = colorRangeToDuty(device, green);
	uint16_t scaledBlue = colorRangeToDuty(device, blue);

	device->writePwm(scaledRed, scaledGreen, scaledBlue);
	device->tickOfColorChange = device->tick;
}

bool rgbInit(RGB *device, RGBWritePwm *writeFn, uint16_t period) {
	if (!device || !writeFn) {
		return false;
	}

	device->writePwm = writeFn;
	device->period = period;

	device->tick = 0;
	device->tickOfColorChange = 0;
	device->showingTransientStatus = false;
	device->userRed = 0;
	device->userGreen = 0;
	device->userBlue = 0;
	return true;
}

void rgbTask(RGB *device, uint16_t tick) {
	if (!device) {
		return;
	}

	device->tick = tick;

	// show status color for 75 ticks, then switch back to user color
	if (device->showingTransientStatus && device->tick >= device->tickOfColorChange + 75U) {
		showColor(device, device->userRed, device->userGreen, device->userBlue, false);
	}
}

void rgbShowNoColor(RGB *device) {
	rgbShowUserColor(device, 0, 0, 0);
}

void rgbShowUserColor(RGB *device, uint8_t red, uint8_t green, uint8_t blue) {
	if (!device) {
		return;
	}

	device->userRed = red;
	device->userGreen = green;
	device->userBlue = blue;

	// only change color if not showing transient status, will show after
	if (!device->showingTransientStatus) {
		showColor(device, red, green, blue, false);
	}
}

void rgbShowSuccess(RGB *device) {
	showColor(device, 10, 10, 10, true);
}

void rgbShowLocked(RGB *device) {
	showColor(device, 0, 0, 20, false);
}

void rgbShowShutdown(RGB *device) {
	showColor(device, 20, 20, 20, true);
}

void rgbShowNotCharging(RGB *device) {
	showColor(device, 10, 0, 10, true);
}

void rgbShowConstantCurrentCharging(RGB *device) {
	showColor(device, 2, 0, 0, true);
}

void rgbShowConstantVoltageCharging(RGB *device) {
	showColor(device, 2, 2, 0, true);
}

void rgbShowDoneCharging(RGB *device) {
	showColor(device, 0, 2, 0, true);
}
