/*
 * rgb.c
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#include "microlight/device/rgb_led.h"

#include <stdbool.h>
#include <stddef.h>

// TODO: multiple priorities, could create a prioritized led resource mutex if it gets more
// complicated button input user defined mode color charging

// Gamma 2.2 correction lookup table: maps linear 0-255 input to corrected 0-255 output.
// Computed as: round(pow(i / 255.0, 2.2) * 255.0) for i in 0..255
static const uint8_t gammaLUT[256] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,
    1,   1,   1,   1,   1,   1,   2,   2,   2,   2,   2,   2,   2,   3,   3,   3,   3,   3,   4,
    4,   4,   4,   5,   5,   5,   5,   6,   6,   6,   6,   7,   7,   7,   8,   8,   8,   9,   9,
    9,   10,  10,  11,  11,  11,  12,  12,  13,  13,  13,  14,  14,  15,  15,  16,  16,  17,  17,
    18,  18,  19,  19,  20,  20,  21,  22,  22,  23,  23,  24,  25,  25,  26,  26,  27,  28,  28,
    29,  30,  30,  31,  32,  33,  33,  34,  35,  35,  36,  37,  38,  39,  39,  40,  41,  42,  43,
    43,  44,  45,  46,  47,  48,  49,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,
    61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  73,  74,  75,  76,  77,  78,  79,  81,
    82,  83,  84,  85,  87,  88,  89,  90,  91,  93,  94,  95,  97,  98,  99,  100, 102, 103, 105,
    106, 107, 109, 110, 111, 113, 114, 116, 117, 119, 120, 121, 123, 124, 126, 127, 129, 130, 132,
    133, 135, 137, 138, 140, 141, 143, 145, 146, 148, 149, 151, 153, 154, 156, 158, 159, 161, 163,
    165, 166, 168, 170, 172, 173, 175, 177, 179, 181, 182, 184, 186, 188, 190, 192, 194, 196, 197,
    199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 234, 236,
    238, 240, 242, 244, 246, 248, 251, 253, 255,
};

// Scale a gamma-corrected 0-255 color value to a PWM duty cycle in [0, period + 1].
//
// Conceptually: duty = corrected * ((period + 1) / 255)
// The +1 is intentional: mapping 255 to period would land exactly on ARR, while mapping 255 to
// period + 1 lets the compare exceed ARR so full-scale behaves as a 100% duty cycle in PWM mode.
// Each color step maps to one increment of the PWM range. However, integer truncation
// in ((period + 1) / 255) loses precision for small periods, so we rearrange to multiply first:
//   duty = (corrected * (period + 1)) / 255
//
// The division by 255 is then replaced with a multiply-shift approximation:
//   x / 255 == (x * 0x8081) >> 23, exact for x in [0, 130559].
// The intermediate product (x * 0x8081) must fit in a uint32_t, which constrains max period:
//   255 * (period + 1) * 0x8081 <= UINT32_MAX  =>  period <= 510
static uint16_t colorRangeToDuty(const RGBLed *device, uint8_t value) {
    uint8_t corrected = gammaLUT[value];
    uint32_t product = (uint32_t)corrected * (device->period + 1U);
    return (uint16_t)((product * 0x8081U) >> 23);
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

bool rgbInit(RGBLed *device, RGBWritePwm writePwm, uint16_t period) {
    if (!device || !writePwm) {
        return false;
    }

    // Max period for colorRangeToDuty: 255 * (period + 1) * 0x8081 must fit in uint32_t.
    // 255 * 511 * 0x8081 = 4,286,644,335 <= UINT32_MAX, but 255 * 512 * 0x8081 overflows,
    // so period must be <= 510.
    if (period > 510) {
        return false;
    }

    device->writePwm = writePwm;
    device->period = period;

    device->ms = 0;
    device->msOfColorChange = 0;
    device->showingTransientStatus = false;
    device->userRed = 0;
    device->userGreen = 0;
    device->userBlue = 0;
    return true;
}

void rgbTransientTask(RGBLed *device, uint32_t milliseconds) {
    if (!device) {
        return;
    }

    device->ms = milliseconds;

    uint32_t elapsedMillis = milliseconds - device->msOfColorChange;

    // show status color for 300 milliseconds, then switch back to user color
    if (device->showingTransientStatus && elapsedMillis > 300) {
        showColor(device, device->userRed, device->userGreen, device->userBlue, false);
    }
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
    if (!device->showingTransientStatus) {
        showColor(device, red, green, blue, false);
    }
}

void rgbShowSuccess(RGBLed *device) {
    showColor(device, 50, 50, 50, true);
}

void rgbShowLocked(RGBLed *device) {
    showColor(device, 0, 0, 65, false);
}

void rgbShowShutdown(RGBLed *device) {
    showColor(device, 65, 65, 65, true);
}

void rgbShowNotCharging(RGBLed *device) {
    showColor(device, 50, 0, 50, true);
}

void rgbShowConstantCurrentCharging(RGBLed *device) {
    showColor(device, 25, 0, 0, true);
}

void rgbShowConstantVoltageCharging(RGBLed *device) {
    showColor(device, 25, 25, 0, true);
}

void rgbShowDoneCharging(RGBLed *device) {
    showColor(device, 0, 25, 0, true);
}
