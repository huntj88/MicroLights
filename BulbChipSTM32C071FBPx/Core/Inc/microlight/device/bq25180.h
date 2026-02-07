/*
 * bq25180.h
 *
 *  Created on: Feb 1, 2025
 *      Author: jameshunt
 */

#ifndef INC_BQ25180_H_
#define INC_BQ25180_H_

#include <stdbool.h>
#include <stdint.h>
#include "microlight/device/i2c.h"
#include "microlight/device/rgb_led.h"
#include "microlight/model/log.h"

#define BQ25180_I2CADDR_DEFAULT 0x6A

#define BQ25180_STAT0 0x00
#define BQ25180_STAT1 0x01
#define BQ25180_FLAG0 0x02
#define BQ25180_VBAT_CTRL 0x3
#define BQ25180_ICHG_CTRL 0x4
#define BQ25180_CHARGECTRL0 0x5
#define BQ25180_CHARGECTRL1 0x6
#define BQ25180_IC_CTRL 0x7
#define BQ25180_TMR_ILIM 0x8
#define BQ25180_SHIP_RST 0x9
#define BQ25180_SYS_REG 0xA
#define BQ25180_TS_CONTROL 0xB
#define BQ25180_MASK_ID 0xC

#define BQ25180_JSON_BUFFER_SIZE 287  // unit test ensures we update this if the size changes

#define STAT0_DEFAULT 0b00000000        // Default value for STAT0 register (Offset = 0x0)
#define STAT1_DEFAULT 0b00000000        // Default value for STAT1 register (Offset = 0x1)
#define FLAG0_DEFAULT 0b00000000        // Default value for FLAG0 register (Offset = 0x2)
#define VBAT_CTRL_DEFAULT 0b01000110    // Default value for VBAT_CTRL register (Offset = 0x3)
#define ICHG_CTRL_DEFAULT 0b00000101    // Default value for ICHG_CTRL register (Offset = 0x4)
#define CHARGECTRL0_DEFAULT 0b00101100  // Default value for CHARGECTRL0 register (Offset = 0x5)
#define CHARGECTRL1_DEFAULT 0b01010110  // Default value for CHARGECTRL1 register (Offset = 0x6)
#define IC_CTRL_DEFAULT 0b10000100      // Default value for IC_CTRL register (Offset = 0x7)
#define TMR_ILIM_DEFAULT 0b01001101     // Default value for TMR_ILIM register (Offset = 0x8)
#define SHIP_RST_DEFAULT 0b00010001     // Default value for SHIP_RST register (Offset = 0x9)
#define SYS_REG_DEFAULT 0b01000000      // Default value for SYS_REG register (Offset = 0xA)
#define TS_CONTROL_DEFAULT 0b00000000   // Default value for TS_CONTROL register (Offset = 0xB)
#define MASK_ID_DEFAULT 0b11000000      // Default value for MASK_ID register (Offset = 0xC)

enum ChargeState { notConnected, notCharging, constantCurrent, constantVoltage, done };

typedef struct BQ25180 {
    I2CReadRegisters readRegisters;
    I2CWriteRegister writeRegister;
    Log log;
    uint8_t devAddress;
    RGBLed *caseLed;

    enum ChargeState chargingState;
    uint32_t checkedAtMs;
} BQ25180;

typedef struct BQ25180Registers {
    uint8_t stat0;
    uint8_t stat1;
    uint8_t flag0;
    uint8_t vbat_ctrl;
    uint8_t ichg_ctrl;
    uint8_t chargectrl0;
    uint8_t chargectrl1;
    uint8_t ic_ctrl;
    uint8_t tmr_ilim;
    uint8_t ship_rst;
    uint8_t sys_reg;
    uint8_t ts_control;
    uint8_t mask_id;
} BQ25180Registers;

typedef struct ChargerTaskFlags {
    bool interruptTriggered;
    bool unplugLockEnabled;
    bool chargeLedEnabled;
    bool serialEnabled;
} ChargerTaskFlags;

bool bq25180Init(
    BQ25180 *chargerIC,
    I2CReadRegisters readRegisters,
    I2CWriteRegister writeRegister,
    uint8_t devAddress,
    Log log,
    RGBLed *caseLed);

void chargerTask(BQ25180 *chargerIC, uint32_t milliseconds, ChargerTaskFlags flags);
void lock(BQ25180 *chargerIC);
enum ChargeState getChargingState(BQ25180 *chargerIC);

// TODO: Handle interrupts from bq25180 and check status/fault registers
//       - send errors over usb to app

#endif /* INC_BQ25180_H_ */
