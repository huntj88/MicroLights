/*
 * chip_state.h
 *
 *  Created on: Jun 29, 2025
 *      Author: jameshunt
 */

#ifndef INC_CHIP_STATE_H_
#define INC_CHIP_STATE_H_

#include <stdint.h>

#include "device/bq25180.h"
#include "device/button.h"
#include "device/mc3479.h"
#include "device/rgb_led.h"
#include "json/command_parser.h"
#include "mode_manager.h"
#include "model/serial.h"
#include "settings_manager.h"

void configureChipState(
    ModeManager *modeManager,
    ChipSettings *settings,
    Button *button,
    BQ25180 *chargerIC,
    MC3479 *accel,
    RGBLed *caseLed,
    WriteToUsbSerial *writeUsbSerial,
    void (*writeBulbLedPin)(uint8_t state),
    uint32_t (*convertTicksToMs)(uint32_t ticks),
    void (*startLedTimers)(),
    void (*stopLedTimers)());

void stateTask();
void chipTickInterrupt();
void autoOffTimerInterrupt();

#endif /* INC_CHIP_STATE_H_ */
