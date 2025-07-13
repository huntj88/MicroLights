/*
 * chip_state.h
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#ifndef INC_CHIP_STATE_H_
#define INC_CHIP_STATE_H_

#include "bulb_json.h"

void setInitialState();
void cdc_task();

void setClickStarted();
void handleButtonInput(void (*shutdown)());
void modeTimerInterrupt();

#endif /* INC_CHIP_STATE_H_ */
