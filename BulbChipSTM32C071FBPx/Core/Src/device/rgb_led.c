/*
 * rgb.c
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#include "device/rgb_led.h"

#include <stdbool.h>
#include <stddef.h>

// TODO: OSSPEEDR register settings for reduced potential EMI on PWM outputs? lower slew rate if not
// affecting color quality
// TODO: OSSPEEDR on bulb io as well

// TODO: multiple priorities, could create a prioritized led resource mutex if it gets more
// complicated button input user defined mode color charging

static uint16_t colorRangeToDuty(const RGBLed *device, uint8_t value) {
    uint16_t increment = device->period / 255U;
    return value * increment;
}

// TODO: move transient side effect to different function
// expect red, green, blue to be in range of 0 to 255
static void showColor(RGBLed *device, uint8_t red, uint8_t green, uint8_t blue, bool transient) {
    if (!device || !device->writePwm) {
        return;
    }

    device->showingTransientStatus = transient;

    uint16_t scaledRed = colorRangeToDuty(device, red);
    uint16_t scaledGreen = colorRangeToDuty(device, green);
    uint16_t scaledBlue = colorRangeToDuty(device, blue);

    device->writePwm(scaledRed, scaledGreen, scaledBlue);
    device->msOfColorChange = device->ms;
}

bool rgbInit(RGBLed *device, RGBWritePwm *writeFn, uint16_t period) {
    if (!device || !writeFn) {
        return false;
    }

    device->writePwm = writeFn;
    device->period = period;

    device->ms = 0;
    device->msOfColorChange = 0;
    device->showingTransientStatus = false;
    device->userRed = 0;
    device->userGreen = 0;
    device->userBlue = 0;
    return true;
}

// static void sinColorRange(uint16_t tick, uint8_t incrementAt, uint8_t *color) {
//	if (tick % incrementAt == 0) {
//		uint16_t sinCounter = tick / incrementAt;
//		double waveProgress = sinCounter / 10.0;
//		double zeroToOneY = (sin(waveProgress) + 1.0) / 2.0;
//		*color = (uint8_t)(zeroToOneY * 255.0);
//	}
// }

// static bool isRainbowModeExperiment(RGB *device) {
//	// TODO: debug parsing to understand why #ffffff is not 255,255,255
//	return device->userRed > 240 && device->userGreen > 240 && device->userGreen > 240;
// }

// static void rainbowModeExperiment(RGB *device, uint16_t tick) {
//	static uint8_t r,g,b;
//
//	if (isRainbowModeExperiment(device)) {
//		sinColorRange(tick, 11, &r);
//		sinColorRange(tick, 17, &g);
//		sinColorRange(tick, 23, &b);
//
//		if (tick % 20 == 0) {
//			showColor(device, r, g, b, false);
//		}
//	}
// }

void rgbTask(RGBLed *device, uint32_t milliseconds) {
    if (!device) {
        return;
    }

    device->ms = milliseconds;

    uint32_t elapsedMillis = milliseconds - device->msOfColorChange;

    // show status color for 300 milliseconds, then switch back to user color
    if (device->showingTransientStatus && elapsedMillis > 300) {
        showColor(device, device->userRed, device->userGreen, device->userBlue, false);
    }

    //	// just for fun
    //	rainbowModeExperiment(device, tick);
}

void rgbShowNoColor(RGBLed *device) {
    rgbShowUserColor(device, 0, 0, 0);
}

void rgbShowUserColor(RGBLed *device, uint8_t red, uint8_t green, uint8_t blue) {
    if (!device) {
        return;
    }

    device->userRed = red;
    device->userGreen = green;
    device->userBlue = blue;

    // only change color if not showing transient status, will show after
    if (!device->showingTransientStatus) {  // && !isRainbowModeExperiment(device)) {
        showColor(device, red, green, blue, false);
    }
}

void rgbShowSuccess(RGBLed *device) {
    showColor(device, 10, 10, 10, true);
}

void rgbShowLocked(RGBLed *device) {
    showColor(device, 0, 0, 20, false);
}

void rgbShowShutdown(RGBLed *device) {
    showColor(device, 20, 20, 20, true);
}

void rgbShowNotCharging(RGBLed *device) {
    showColor(device, 10, 0, 10, true);
}

void rgbShowConstantCurrentCharging(RGBLed *device) {
    showColor(device, 2, 0, 0, true);
}

void rgbShowConstantVoltageCharging(RGBLed *device) {
    showColor(device, 2, 2, 0, true);
}

void rgbShowDoneCharging(RGBLed *device) {
    showColor(device, 0, 2, 0, true);
}
