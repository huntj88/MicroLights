/*
 * i2c_log_decorate.c
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include "microlight/i2c_log_decorate.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// TODO: change name to log_decorate and decorate more hardware interfaces?

static void internalLog(
    WriteToSerial *logFunc, bool *enabled, const char *operation, uint8_t devAddress, uint8_t reg) {
    if (enabled && *enabled && logFunc) {
        char buf[64];
        snprintf(
            buf,
            sizeof(buf),
            "I2C FAIL: %s addr=0x%02X reg=0x%02X\r\n",
            operation,
            devAddress,
            reg);
        logFunc(buf, strlen(buf));
    }
}

void i2cDecoratedWrite(
    uint8_t devAddress,
    uint8_t reg,
    uint8_t value,
    I2CWriteRegisterChecked *rawFunc,
    bool *enableFlag,
    WriteToSerial *logFunc) {
    if (rawFunc && !rawFunc(devAddress, reg, value)) {
        internalLog(logFunc, enableFlag, "Write", devAddress, reg);
    }
}

bool i2cDecoratedReadRegisters(
    uint8_t devAddress,
    uint8_t startReg,
    uint8_t *buf,
    size_t len,
    I2CReadRegisters *rawFunc,
    bool *enableFlag,
    WriteToSerial *logFunc) {
    if (rawFunc && rawFunc(devAddress, startReg, buf, len)) {
        return true;
    }
    internalLog(logFunc, enableFlag, "ReadMul", devAddress, startReg);
    return false;
}
