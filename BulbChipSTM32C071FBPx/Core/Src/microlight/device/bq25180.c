/*
 * bq25180.c
 *
 *  Created on: Feb 1, 2025
 *      Author: jameshunt
 */

#include "microlight/device/bq25180.h"

#include <stdio.h>
#include <string.h>

// more than one chargerIC not likely,
// if handling multiple chargers would need to pass in function pointer that get bool for specific
// interrupt variable
static volatile bool readChargerNow = false;

// Forward declarations
static void readAllRegistersJson(BQ25180 *chargerIC, char jsonOutput[], uint32_t len);
static BQ25180Registers readAllRegisters(BQ25180 *chargerIC);
static void configureChargerIC(BQ25180 *chargerIC);
static void configureRegister_IC_CTRL(BQ25180 *chargerIC);
static void configureRegister_ICHG_CTRL(BQ25180 *chargerIC);
static void configureRegister_VBAT_CTRL(BQ25180 *chargerIC);
static void configureRegister_CHARGECTRL1(BQ25180 *chargerIC);
static void configureRegister_SYS_REG(BQ25180 *chargerIC);
static void configureRegister_MASK_ID(BQ25180 *chargerIC);
static void showChargingState(BQ25180 *chargerIC, enum ChargeState state);
static void enableShipMode(BQ25180 *chargerIC);
static void hardwareReset(BQ25180 *chargerIC);
static void byteToBinary(uint8_t num, char *buf);
static void bq25180regsToJson(BQ25180Registers registers, char jsonOutput[], uint32_t len);

// =================================================================================================
// Public Interface
// =================================================================================================

bool bq25180Init(
    BQ25180 *chargerIC,
    I2CReadRegisters *readRegsCb,
    I2CWriteRegister *writeCb,
    uint8_t devAddress,
    Log *log,
    RGBLed *caseLed,
    void (*enableTimers)(bool enable)) {
    if (!chargerIC || !readRegsCb || !writeCb || !log || !caseLed || !enableTimers) {
        return false;
    }

    chargerIC->readRegisters = readRegsCb;
    chargerIC->writeRegister = writeCb;
    chargerIC->devAddress = devAddress;
    chargerIC->log = log;
    chargerIC->caseLed = caseLed;
    chargerIC->enableTimers = enableTimers;

    chargerIC->chargingState = notConnected;
    chargerIC->checkedAtMs = 0;

    configureChargerIC(chargerIC);

    return true;
}

void handleChargerInterrupt() {
    readChargerNow = 1;
}

void chargerTask(
    BQ25180 *chargerIC,
    uint32_t milliseconds,
    bool unplugLockEnabled,
    bool ledEnabled,
    bool serialEnabled) {
    enum ChargeState previousState = chargerIC->chargingState;
    uint32_t elapsedMillis = 0;

    if (chargerIC->checkedAtMs != 0) {
        elapsedMillis = milliseconds - chargerIC->checkedAtMs;
    }

    // charger i2c watchdog timer will reset if not communicated
    // with for 40 seconds, and 15 seconds after plugged in.
    if (elapsedMillis > 30000 || chargerIC->checkedAtMs == 0) {
        if (serialEnabled) {
            char registerJson[BQ25180_JSON_BUFFER_SIZE];
            readAllRegistersJson(chargerIC, registerJson, sizeof(registerJson));
            chargerIC->log(registerJson, strlen(registerJson));
        }

        chargerIC->chargingState = getChargingState(chargerIC);
        chargerIC->checkedAtMs = milliseconds;
    }

    // flash charging state to user every ~1 second (1024ms, 2^10)
    if (ledEnabled && chargerIC->chargingState != notConnected && (milliseconds & 0x3FF) < 50) {
        showChargingState(chargerIC, chargerIC->chargingState);
    }

    if (readChargerNow) {
        readChargerNow = false;
        enum ChargeState state = getChargingState(chargerIC);
        chargerIC->chargingState = state;

        bool wasDisconnected = previousState != notConnected && state == notConnected;
        if (milliseconds != 0 && wasDisconnected && unplugLockEnabled) {
            // if in fake off mode and power is unplugged, put into ship mode
            lock(chargerIC);
        }

        // only update LED from interrupt when plugged in for immediate feedback.
        bool wasConnected = previousState == notConnected && state != notConnected;
        if (wasConnected && ledEnabled) {
            chargerIC->enableTimers(true);  // timers needed to show charging status led
            showChargingState(chargerIC, state);
        }
    }
}

enum ChargeState getChargingState(BQ25180 *chargerIC) {
    uint8_t regResult = 0;
    if (!chargerIC->readRegisters(chargerIC->devAddress, BQ25180_STAT0, &regResult, 1)) {
        return notConnected;
    }

    if ((regResult & 0b01000000) > 0) {
        if ((regResult & 0b00100000) > 0) {
            return done;
        }
        return constantVoltage;
    }

    if ((regResult & 0b00100000) > 0) {
        return constantCurrent;
    }

    // check if plugged in
    if ((regResult & 0b00000001) > 0) {
        return notCharging;
    }
    return notConnected;
}

void lock(BQ25180 *chargerIC) {
    enum ChargeState state = getChargingState(chargerIC);
    if (state == notConnected) {
        enableShipMode(chargerIC);
    } else {
        hardwareReset(chargerIC);
    }
}

// =================================================================================================
// Private Helpers - State & UI
// =================================================================================================

static void showChargingState(BQ25180 *chargerIC, enum ChargeState state) {
    switch (state) {
        case notConnected:
            // do nothing
            break;
        case notCharging:
            rgbShowNotCharging(chargerIC->caseLed);
            break;
        case constantCurrent:
            rgbShowConstantCurrentCharging(chargerIC->caseLed);
            break;
        case constantVoltage:
            rgbShowConstantVoltageCharging(chargerIC->caseLed);
            break;
        case done:
            rgbShowDoneCharging(chargerIC->caseLed);
            break;
    }
}

// =================================================================================================
// Private Helpers - Configuration
// =================================================================================================

static void configureChargerIC(BQ25180 *chargerIC) {
    configureRegister_IC_CTRL(chargerIC);
    configureRegister_ICHG_CTRL(chargerIC);
    configureRegister_VBAT_CTRL(chargerIC);
    configureRegister_CHARGECTRL1(chargerIC);
    configureRegister_SYS_REG(chargerIC);
    configureRegister_MASK_ID(chargerIC);
}

static void configureRegister_IC_CTRL(BQ25180 *chargerIC) {
    uint8_t newConfig = IC_CTRL_DEFAULT;
    uint8_t tsEnabledMask = 0b10000000;
    newConfig &= ~tsEnabledMask;  // disable ts current changes (no thermistor on my project?)
    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_IC_CTRL, newConfig);
}

static void configureRegister_ICHG_CTRL(BQ25180 *chargerIC) {
    // enable charging = bit 7 in data sheet 0
    // 70 milliamp max charge current
    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_ICHG_CTRL, 0b00100010);
}

static void configureRegister_VBAT_CTRL(BQ25180 *chargerIC) {
    // 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000
    //	 chargerIC->writeRegister(chargerIC, BQ25180_VBAT_CTRL, 0b01010000);

    // 4.4v, (3.5v) + (90 * 10mV), 90 = 0b1011010
    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_VBAT_CTRL, 0b01011010);
}

static void configureRegister_CHARGECTRL1(BQ25180 *chargerIC) {
    // Battery Discharge Current Limit
    // 2b00 = 500mA

    // Battery Undervoltage LockOut Falling Threshold.
    // 3b000 = 3.0V

    // Mask Charging Status Interrupt = ON 1b0
    // Mask ILIM Fault Interrupt = OFF 1b1
    // Mask VINDPM and VDPPM Interrupt = OFF 1b1, TODO: turn back on?, 1b0

    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_CHARGECTRL1, 0b00000011);
}

static void configureRegister_SYS_REG(BQ25180 *chargerIC) {
    uint8_t newConfig = SYS_REG_DEFAULT;
    uint8_t vinWatchdogMask = 0b00000010;
    uint8_t systemRegulationMask = 0b11100000;

    // enabled vin watchdog hardware reset, if no i2c with 15 seconds of vin
    newConfig |= vinWatchdogMask;

    // set all system regulation bits to 0 to enable battery tracking mode
    // Idea being that less voltage conversions means higher efficiency?
    // I Have a 3.3v buck regulator between chargingIC sys and mcu vin
    // lowest SYS voltage will be 3.8v even if battery is at 3.0v
    newConfig &= ~systemRegulationMask;

    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_SYS_REG, newConfig);
}

static void configureRegister_MASK_ID(BQ25180 *chargerIC) {
    // TS_INT_MASK: Mask or enable the TS interrupt.
    //   1b0: Enable TS interrupt
    //   1b1: Mask TS interrupt

    // TREG_INT_MASK: Mask or enable the TREG interrupt.
    //   1b0: Enable TREG interrupt
    //   1b1: Mask TREG interrupt

    // BAT_INT_MASK: Mask or enable BATOCP and BUVLO interrupts.
    //   1b0: Enable BATOCP and BUVLO interrupts
    //   1b1: Mask BATOCP and BUVLO interrupts

    // PG_INT_MASK: Mask or enable PG and VINOVP interrupts.
    //   1b0: Enable PG and VINOVP interrupts
    //   1b1: Mask PG and VINOVP interrupts

    // Device_ID: A 4-bit field indicating the device ID.
    //   4b0000: Device ID for the BQ25180.

    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_MASK_ID, 0b00000000);
}

// =================================================================================================
// Private Helpers - Registers & JSON
// =================================================================================================

static void readAllRegistersJson(BQ25180 *chargerIC, char jsonOutput[], uint32_t len) {
    BQ25180Registers registerValues = readAllRegisters(chargerIC);
    bq25180regsToJson(registerValues, jsonOutput, len);
}

static BQ25180Registers readAllRegisters(BQ25180 *chargerIC) {
    BQ25180Registers registerValues = {0};
    uint8_t rxBuffer[13];

    if (chargerIC->readRegisters(chargerIC->devAddress, BQ25180_STAT0, rxBuffer, 13)) {
        registerValues.stat0 = rxBuffer[0];
        registerValues.stat1 = rxBuffer[1];
        registerValues.flag0 = rxBuffer[2];
        registerValues.vbat_ctrl = rxBuffer[3];
        registerValues.ichg_ctrl = rxBuffer[4];
        registerValues.chargectrl0 = rxBuffer[5];
        registerValues.chargectrl1 = rxBuffer[6];
        registerValues.ic_ctrl = rxBuffer[7];
        registerValues.tmr_ilim = rxBuffer[8];
        registerValues.ship_rst = rxBuffer[9];
        registerValues.sys_reg = rxBuffer[10];
        registerValues.ts_control = rxBuffer[11];
        registerValues.mask_id = rxBuffer[12];
    }

    return registerValues;
}

static void bq25180regsToJson(const BQ25180Registers registers, char jsonOutput[], uint32_t len) {
    if (!jsonOutput) {
        return;
    }
    char bins[13][9];
    byteToBinary(registers.stat0, bins[0]);
    byteToBinary(registers.stat1, bins[1]);
    byteToBinary(registers.flag0, bins[2]);
    byteToBinary(registers.vbat_ctrl, bins[3]);
    byteToBinary(registers.ichg_ctrl, bins[4]);
    byteToBinary(registers.chargectrl0, bins[5]);
    byteToBinary(registers.chargectrl1, bins[6]);
    byteToBinary(registers.ic_ctrl, bins[7]);
    byteToBinary(registers.tmr_ilim, bins[8]);
    byteToBinary(registers.ship_rst, bins[9]);
    byteToBinary(registers.sys_reg, bins[10]);
    byteToBinary(registers.ts_control, bins[11]);
    byteToBinary(registers.mask_id, bins[12]);

    snprintf(
        jsonOutput,
        len,
        "{\"stat0\":\"%s\",\"stat1\":\"%s\",\"flag0\":\"%s\",\"vbat_ctrl\":\"%s\","
        "\"ichg_ctrl\":\"%s\",\"chargectrl0\":\"%s\",\"chargectrl1\":\"%s\","
        "\"ic_ctrl\":\"%s\",\"tmr_ilim\":\"%s\",\"ship_rst\":\"%s\","
        "\"sys_reg\":\"%s\",\"ts_control\":\"%s\",\"mask_id\":\"%s\"}\n",
        bins[0],
        bins[1],
        bins[2],
        bins[3],
        bins[4],
        bins[5],
        bins[6],
        bins[7],
        bins[8],
        bins[9],
        bins[10],
        bins[11],
        bins[12]);
}

// Convert a byte to an 8-character binary string that is null-terminated
// Buffer 'buf' must be at least 9 bytes long
static void byteToBinary(uint8_t num, char *buf) {
    for (int i = 0; i < 8; i++) {
        buf[i] = (num & (1 << (7 - i))) ? '1' : '0';
    }
    buf[8] = '\0';
}

// =================================================================================================
// Private Helpers - Power Management
// =================================================================================================

// power must be unplugged to enter ship mode.
static void enableShipMode(BQ25180 *chargerIC) {
    // REG_RST - Software Reset
    // 1b0 = Do nothing
    // 1b1 = Software Reset

    // EN_RST_SHIP_1 - Shipmode Enable and Hardware Reset, once bits set, power must be unplugged to
    // enter ship mode. 2b00 = Do nothing 2b01 = Enable shutdown mode with wake on adapter insert
    // only 2b10 = Enable shipmode with wake on button press or adapter insert 2b11 = Hardware Reset

    // PB_LPRESS_ACTION_1 - Pushbutton long press action
    // 2b00 = Do nothing
    // 2b01 = Hardware Reset
    // 2b10 = Enable shipmode
    // 2b11 = Enable shutdown mode

    // WAKE1_TMR - Wake 1 Timer Set
    // 1b0 = 300ms
    // 1b1 = 1s

    // WAKE2_TMR - Wake 2 Timer Set
    // 1b0 = 2s
    // 1b1 = 3s

    // EN_PUSH - Enable Push Button and Reset Function on Battery Only
    // 1b0 = Disable
    // 1b1 = Enable

    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_SHIP_RST, 0b01000001);
}

static void hardwareReset(BQ25180 *chargerIC) {
    chargerIC->writeRegister(chargerIC->devAddress, BQ25180_SHIP_RST, 0b01100001);
}
