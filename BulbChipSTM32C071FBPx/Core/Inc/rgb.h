/*
 * rgb.h
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#ifndef INC_RGB_H_
#define INC_RGB_H_

#include <stdint.h>
#include <stdbool.h>

typedef void RGBWritePwm(uint16_t redDuty, uint16_t greenDuty, uint16_t blueDuty);

typedef struct RGB {
	RGBWritePwm *writePwm;
	uint16_t period; // TODO: set period from config?

	uint16_t tick;
	uint16_t tickOfColorChange;
	bool showingTransientStatus;
	uint8_t userRed;
	uint8_t userGreen;
	uint8_t userBlue;
} RGB;

bool rgbInit(RGB *device, RGBWritePwm *writeFn, uint16_t period);

void rgbTask(RGB *device, uint16_t tick);
void rgbShowNoColor(RGB *device);
void rgbShowUserColor(RGB *device, uint8_t red, uint8_t green, uint8_t blue);
void rgbShowSuccess(RGB *device);
void rgbShowLocked(RGB *device);
void rgbShowShutdown(RGB *device);
void rgbShowNotCharging(RGB *device);
void rgbShowConstantCurrentCharging(RGB *device);
void rgbShowConstantVoltageCharging(RGB *device);
void rgbShowDoneCharging(RGB *device);

#endif /* INC_RGB_H_ */
