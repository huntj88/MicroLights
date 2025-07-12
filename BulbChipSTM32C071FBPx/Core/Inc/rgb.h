/*
 * rgb.h
 *
 *  Created on: Jul 3, 2025
 *      Author: jameshunt
 */

#ifndef INC_RGB_H_
#define INC_RGB_H_

// expect red, green, blue to be in range of 0 to 255
void showColor(uint8_t red, uint8_t green, uint8_t blue);

void showSuccess();
void showFailure();
void showLocked();
void showShutdown();
void showNoColor();
void rgb_task();

#endif /* INC_RGB_H_ */
