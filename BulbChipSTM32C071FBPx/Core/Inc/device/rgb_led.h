/*
 * rgb.h
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#ifndef INC_RGB_LED_H_
#define INC_RGB_LED_H_

#include <stdint.h>
#include <stdbool.h>

typedef void RGBWritePwm(uint16_t redDuty, uint16_t greenDuty, uint16_t blueDuty);

typedef struct RGBLed {
	RGBWritePwm *writePwm;
	uint16_t period; // TODO: set period from config?
	void (*startLedTimers)();
	void (*stopLedTimers)();

	uint16_t ms;
	uint16_t msOfColorChange;
	bool showingTransientStatus;
	uint8_t userRed;
	uint8_t userGreen;
	uint8_t userBlue;
} RGBLed;

bool rgbInit(RGBLed *device, RGBWritePwm *writeFn, uint16_t period, void (*startLedTimers)(), void (*stopLedTimers)());

void rgbTask(RGBLed *device, uint16_t ms);
void rgbShowNoColor(RGBLed *device);
void rgbShowUserColor(RGBLed *device, uint8_t red, uint8_t green, uint8_t blue);
void rgbShowSuccess(RGBLed *device);
void rgbShowLocked(RGBLed *device);
void rgbShowShutdown(RGBLed *device);
void rgbShowNotCharging(RGBLed *device);
void rgbShowConstantCurrentCharging(RGBLed *device);
void rgbShowConstantVoltageCharging(RGBLed *device);
void rgbShowDoneCharging(RGBLed *device);

#endif /* INC_RGB_LED_H_ */
