/*
 * chip_state.h
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#ifndef INC_CHIP_STATE_H_
#define INC_CHIP_STATE_H_

#include "bulb_json.h"

typedef void WriteToUsbType(uint8_t itf, uint8_t buf[], uint32_t count);

void configureChipState(WriteToUsbType *writeToUsb);
void handleJson(uint8_t buf[], uint32_t count);

void setClickStarted();
void handleButtonInput(void (*shutdown)());
void modeTimerInterrupt();

#endif /* INC_CHIP_STATE_H_ */
