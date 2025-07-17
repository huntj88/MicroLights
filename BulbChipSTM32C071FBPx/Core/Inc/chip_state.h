/*
 * chip_state.h
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#ifndef INC_CHIP_STATE_H_
#define INC_CHIP_STATE_H_

#include "bulb_json.h"
#include "bq25180.h"

typedef void WriteToUsbSerial(uint8_t itf, uint8_t buf[], uint32_t count);

void configureChipState(BQ25180 *chargerIC, WriteToUsbSerial *writeUsbSerial, void (*enterDFU)(), uint8_t (*readButtonPin)());
void handleJson(uint8_t buf[], uint32_t count);

void setClickStarted();
void stateTask();
void modeTimerInterrupt();
void handleChargerInterrupt();


#endif /* INC_CHIP_STATE_H_ */
