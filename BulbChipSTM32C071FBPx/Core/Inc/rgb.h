/*
 * rgb.h
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#ifndef INC_RGB_H_
#define INC_RGB_H_

void showUserColor(uint8_t red, uint8_t green, uint8_t blue);
void showSuccess();
void showFailure();
void showLocked();
void showShutdown();
void showNoColor();
void showNotCharging();
void showConstantCurrentCharging();
void showConstantVoltageCharging();
void showDoneCharging();
void rgb_task();

#endif /* INC_RGB_H_ */
