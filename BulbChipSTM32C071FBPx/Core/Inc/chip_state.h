/*
 * chip_state.h
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#ifndef INC_CHIP_STATE_H_
#define INC_CHIP_STATE_H_

#include <stdint.h>

#include "device/button.h"
#include "device/bq25180.h"
#include "device/mc3479.h"
#include "device/rgb_led.h"
#include "json/command_parser.h"
#include "mode_manager.h"
#include "settings_manager.h"

typedef void WriteToUsbSerial(uint8_t itf, const char *buf, uint32_t count);

void configureChipState(
		ModeManager *modeManager,
		ChipSettings *settings,
		Button *button,
		BQ25180 *chargerIC,
		MC3479 *accel,
		RGBLed *rgb,
		WriteToUsbSerial *writeUsbSerial,
		void (*enterDFU)(),
//		uint8_t (*readButtonPin)(),
		void (*writeBulbLedPin)(uint8_t state),
		float (*getMillisecondsPerChipTick)(),
		void (*startLedTimers)(),
		void (*stopLedTimers)()
);

// API for command parser
void chip_state_enter_dfu();
void chip_state_write_serial(const char *msg);
void chip_state_show_success();

void setClickStarted();
void stateTask();
void chipTickInterrupt();
// void handleChargerInterrupt();
void autoOffTimerInterrupt();


#endif /* INC_CHIP_STATE_H_ */
