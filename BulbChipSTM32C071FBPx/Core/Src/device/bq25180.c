/*
 * bq25180.c
 *
 *  Created on: Feb 1, 2025
 *      Author: jameshunt
 */

#include "device/bq25180.h"

#include <stdio.h>
#include <string.h>
#include "device/rgb_led.h"

// more than one chargerIC not likely,
// if handling multiple chargers would need to pass in function pointer that get bool for specific
// interrupt variable
static volatile bool readChargerNow = false;

void handleChargerInterrupt() {
    readChargerNow = 1;
}

bool bq25180Init(BQ25180 *chargerIC,
                 BQ25180ReadRegister *readRegCb,
                 BQ25180WriteRegister *writeCb,
                 uint8_t devAddress,
                 WriteToUsbSerial *writeToUsbSerial,
                 RGBLed *caseLed) {
    if (!chargerIC || !readRegCb || !writeCb || !writeToUsbSerial || !caseLed) return false;

    chargerIC->readRegister = readRegCb;
    chargerIC->writeRegister = writeCb;
    chargerIC->devAddress = devAddress;
    chargerIC->writeToUsbSerial = writeToUsbSerial;
    chargerIC->caseLed = caseLed;

    chargerIC->chargingState = notConnected;
    chargerIC->checkedAtMs = 0;

    configureChargerIC(chargerIC);

    return true;
}

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

void chargerTask(BQ25180 *chargerIC, uint32_t ms, bool unplugLockEnabled, bool ledEnabled) {
    enum ChargeState previousState = chargerIC->chargingState;
    uint32_t elapsedMillis = 0;

    if (chargerIC->checkedAtMs != 0) {
        elapsedMillis = ms - chargerIC->checkedAtMs;
    }

    // charger i2c watchdog timer will reset if not communicated
    // with for 40 seconds, and 15 seconds after plugged in.
    if (elapsedMillis > 30000 || chargerIC->checkedAtMs == 0) {
        // char registerJson[256];
        // readAllRegistersJson(chargerIC, registerJson);
        // writeUsbSerial(0, registerJson, strlen(registerJson));
        // printAllRegisters(chargerIC);

        chargerIC->chargingState = getChargingState(chargerIC);
        chargerIC->checkedAtMs = ms;
    }

    // flash charging state to user every ~1 second (1024ms, 2^10)
    if (ledEnabled && chargerIC->chargingState != notConnected && (ms & 0x3FF) < 50) {
        showChargingState(chargerIC, chargerIC->chargingState);
    }

    if (readChargerNow) {
        readChargerNow = false;
        enum ChargeState state = getChargingState(chargerIC);
        chargerIC->chargingState = state;

        bool wasDisconnected = previousState != notConnected && state == notConnected;
        if (ms != 0 && wasDisconnected && unplugLockEnabled) {
            // if in fake off mode and power is unplugged, put into ship mode
            lock(chargerIC);
        }

        // only update LED from interrupt when plugged in for immediate feedback.
        bool wasConnected = previousState == notConnected && state != notConnected;
        if (wasConnected && ledEnabled) {
            chargerIC->caseLed->startLedTimers();  // show charging status led
            showChargingState(chargerIC, state);
        }
    }
}

enum ChargeState getChargingState(BQ25180 *chargerIC) {
    uint8_t regResult = chargerIC->readRegister(chargerIC, BQ25180_STAT0);

    if ((regResult & 0b01000000) > 0) {
        if ((regResult & 0b00100000) > 0) {
            return done;
        } else {
            return constantVoltage;
        }
    } else if ((regResult & 0b00100000) > 0) {
        return constantCurrent;
    }

    // check if plugged in
    if ((regResult & 0b00000001) > 0) {
        return notCharging;
    } else {
        return notConnected;
    }
}

void configureRegister_IC_CTRL(BQ25180 *chargerIC) {
    uint8_t newConfig = IC_CTRL_DEFAULT;
    uint8_t tsEnabledMask = 0b10000000;
    newConfig &= ~tsEnabledMask;  // disable ts current changes (no thermistor on my project?)
    chargerIC->writeRegister(chargerIC, BQ25180_IC_CTRL, newConfig);
}

void configureRegister_ICHG_CTRL(BQ25180 *chargerIC) {
    // enable charging = bit 7 in data sheet 0
    // 70 milliamp max charge current
    chargerIC->writeRegister(chargerIC, BQ25180_ICHG_CTRL, 0b00100010);
}

void configureRegister_VBAT_CTRL(BQ25180 *chargerIC) {
    // 4.3v, (3.5v) + (80 * 10mV), 80 = 0b1010000
    //	 chargerIC->writeRegister(chargerIC, BQ25180_VBAT_CTRL, 0b01010000);

    // 4.4v, (3.5v) + (90 * 10mV), 90 = 0b1011010
    chargerIC->writeRegister(chargerIC, BQ25180_VBAT_CTRL, 0b01011010);
}

void configureRegister_CHARGECTRL1(BQ25180 *chargerIC) {
    // Battery Discharge Current Limit
    // 2b00 = 500mA

    // Battery Undervoltage LockOut Falling Threshold.
    // 3b000 = 3.0V

    // Mask Charging Status Interrupt = ON 1b0
    // Mask ILIM Fault Interrupt = OFF 1b1
    // Mask VINDPM and VDPPM Interrupt = OFF 1b1, TODO: turn back on?, 1b0

    chargerIC->writeRegister(chargerIC, BQ25180_CHARGECTRL1, 0b00000011);
}

void configureRegister_SYS_REG(BQ25180 *chargerIC) {
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

    chargerIC->writeRegister(chargerIC, BQ25180_SYS_REG, newConfig);
}

void configureRegister_MASK_ID(BQ25180 *chargerIC) {
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

    chargerIC->writeRegister(chargerIC, BQ25180_MASK_ID, 0b00000000);
}

void configureChargerIC(BQ25180 *chargerIC) {
    configureRegister_IC_CTRL(chargerIC);
    configureRegister_ICHG_CTRL(chargerIC);
    configureRegister_VBAT_CTRL(chargerIC);
    configureRegister_CHARGECTRL1(chargerIC);
    configureRegister_SYS_REG(chargerIC);
    configureRegister_MASK_ID(chargerIC);
}

void print(BQ25180 *chargerIC, char *stringToPrint) {
    chargerIC->writeToUsbSerial(0, stringToPrint, strlen(stringToPrint));
}

void printBinary(BQ25180 *chargerIC, uint8_t num) {
    char buffer[9] = {0};
    char *bufferPtr = buffer;

    sprintf(bufferPtr + 0, "%d", (num & 0b10000000) > 0 ? 1 : 0);
    sprintf(bufferPtr + 1, "%d", (num & 0b01000000) > 0 ? 1 : 0);
    sprintf(bufferPtr + 2, "%d", (num & 0b00100000) > 0 ? 1 : 0);
    sprintf(bufferPtr + 3, "%d", (num & 0b00010000) > 0 ? 1 : 0);
    sprintf(bufferPtr + 4, "%d", (num & 0b00001000) > 0 ? 1 : 0);
    sprintf(bufferPtr + 5, "%d", (num & 0b00000100) > 0 ? 1 : 0);
    sprintf(bufferPtr + 6, "%d", (num & 0b00000010) > 0 ? 1 : 0);
    sprintf(bufferPtr + 7, "%d", (num & 0b00000001) > 0 ? 1 : 0);

    chargerIC->writeToUsbSerial(0, buffer, sizeof(buffer));
}

static void bq25180regsToJson(const BQ25180Registers r, char *jsonOutput) {
    if (!jsonOutput) {
        return;
    }
    sprintf(jsonOutput,
            "{\"stat0\":%d,\"stat1\":%d,\"flag0\":%d,\"vbat_ctrl\":%d,"
            "\"ichg_ctrl\":%d,\"chargectrl0\":%d,\"chargectrl1\":%d,"
            "\"ic_ctrl\":%d,\"tmr_ilim\":%d,\"ship_rst\":%d,"
            "\"sys_reg\":%d,\"ts_control\":%d,\"mask_id\":%d}\n",
            r.stat0, r.stat1, r.flag0, r.vbat_ctrl, r.ichg_ctrl, r.chargectrl0, r.chargectrl1,
            r.ic_ctrl, r.tmr_ilim, r.ship_rst, r.sys_reg, r.ts_control, r.mask_id);
}

void readAllRegistersJson(BQ25180 *chargerIC, char *jsonOutput) {
    BQ25180Registers registerValues = readAllRegisters(chargerIC);
    bq25180regsToJson(registerValues, jsonOutput);
}

BQ25180Registers readAllRegisters(BQ25180 *chargerIC) {
    BQ25180Registers registerValues;
    registerValues.stat0 = chargerIC->readRegister(chargerIC, BQ25180_STAT0);
    registerValues.stat1 = chargerIC->readRegister(chargerIC, BQ25180_STAT1);
    registerValues.flag0 = chargerIC->readRegister(chargerIC, BQ25180_FLAG0);
    registerValues.vbat_ctrl = chargerIC->readRegister(chargerIC, BQ25180_VBAT_CTRL);
    registerValues.ichg_ctrl = chargerIC->readRegister(chargerIC, BQ25180_ICHG_CTRL);
    registerValues.chargectrl0 = chargerIC->readRegister(chargerIC, BQ25180_CHARGECTRL0);
    registerValues.chargectrl1 = chargerIC->readRegister(chargerIC, BQ25180_CHARGECTRL1);
    registerValues.ic_ctrl = chargerIC->readRegister(chargerIC, BQ25180_IC_CTRL);
    registerValues.tmr_ilim = chargerIC->readRegister(chargerIC, BQ25180_TMR_ILIM);
    registerValues.ship_rst = chargerIC->readRegister(chargerIC, BQ25180_SHIP_RST);
    registerValues.sys_reg = chargerIC->readRegister(chargerIC, BQ25180_SYS_REG);
    registerValues.ts_control = chargerIC->readRegister(chargerIC, BQ25180_TS_CONTROL);
    registerValues.mask_id = chargerIC->readRegister(chargerIC, BQ25180_MASK_ID);
    return registerValues;
}

void printRegister(BQ25180 *chargerIC, uint8_t reg, char *label) {
    uint8_t regValue = chargerIC->readRegister(chargerIC, reg);

    print(chargerIC, "register");
    print(chargerIC, " ");
    print(chargerIC, label);
    print(chargerIC, ": ");

    printBinary(chargerIC, regValue);

    char newLine[1] = "\n";
    chargerIC->writeToUsbSerial(0, newLine, sizeof(newLine));
}

void printAllRegisters(BQ25180 *chargerIC) {
    char newLine[1] = "\n";
    chargerIC->writeToUsbSerial(0, newLine, sizeof(newLine));

    printRegister(chargerIC, BQ25180_STAT0, "STAT0");
    printRegister(chargerIC, BQ25180_STAT1, "STAT1");
    printRegister(chargerIC, BQ25180_FLAG0, "FLAG0");
    printRegister(chargerIC, BQ25180_VBAT_CTRL, "VBAT_CTRL");
    printRegister(chargerIC, BQ25180_ICHG_CTRL, "ICHG_CTRL");
    printRegister(chargerIC, BQ25180_CHARGECTRL0, "CHARGECTRL0");
    printRegister(chargerIC, BQ25180_CHARGECTRL1, "CHARGECTRL1");
    printRegister(chargerIC, BQ25180_IC_CTRL, "IC_CTRL");
    printRegister(chargerIC, BQ25180_TMR_ILIM, "TMR_ILIM");
    printRegister(chargerIC, BQ25180_SHIP_RST, "SHIP_RST");
    printRegister(chargerIC, BQ25180_SYS_REG, "SYS_REG");
    printRegister(chargerIC, BQ25180_TS_CONTROL, "TS_CONTROL");
    printRegister(chargerIC, BQ25180_MASK_ID, "MASK_ID");
}

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

    chargerIC->writeRegister(chargerIC, BQ25180_SHIP_RST, 0b01000001);
}

static void hardwareReset(BQ25180 *chargerIC) {
    chargerIC->writeRegister(chargerIC, BQ25180_SHIP_RST, 0b01100001);
}

void lock(BQ25180 *chargerIC) {
    enum ChargeState state = getChargingState(chargerIC);
    if (state == notConnected) {
        enableShipMode(chargerIC);
    } else {
        hardwareReset(chargerIC);
    }
}
