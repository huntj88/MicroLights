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
#include "mc3479.h"
#include "rgb.h"

typedef void WriteToUsbSerial(uint8_t itf, const char *buf, uint32_t count);

void configureChipState(
		BQ25180 *chargerIC,
		MC3479 *accel,
		RGB *rgb,
		WriteToUsbSerial *writeUsbSerial,
		void (*enterDFU)(),
		uint8_t (*readButtonPin)(),
		void (*writeBulbLedPin)(uint8_t state),
		void (*startLedTimers)(),
		void (*stopLedTimers)()
);

void handleJson(uint8_t buf[], uint32_t count);

void setClickStarted();
void stateTask();
void chipTickInterrupt();
void handleChargerInterrupt();
void autoOffTimerInterrupt();


#endif /* INC_CHIP_STATE_H_ */
