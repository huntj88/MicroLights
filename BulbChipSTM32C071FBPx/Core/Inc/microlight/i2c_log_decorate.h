/*
 * i2c_log_decorate.h
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#ifndef MICROLIGHT_I2C_LOG_DECORATE_H
#define MICROLIGHT_I2C_LOG_DECORATE_H

#include <stdbool.h>
#include "microlight/device/i2c.h"
#include "microlight/model/serial.h"

/**
 * @brief Stateless helper that executes a write transaction checking for errors and logging them.
 *
 * @param devAddress I2C device address
 * @param reg Register to write to
 * @param value Value to write
 * @param rawFunc The underlying I2C function to call
 * @param enableFlag Pointer to the boolean flag determining if logging is active
 * @param logFunc The logging function to use (e.g. WriteToSerial)
 */
void i2cDecoratedWrite(
    uint8_t devAddress,
    uint8_t reg,
    uint8_t value,
    I2CWriteRegisterChecked *rawFunc,
    const bool *enableFlag,
    WriteToSerial *logFunc);

/**
 * @brief Stateless helper that executes a multi-byte read transaction checking for errors and
 * logging them.
 *
 * @return true if successful, false otherwise
 */
bool i2cDecoratedReadRegisters(
    uint8_t devAddress,
    uint8_t startReg,
    uint8_t *buf,
    size_t len,
    I2CReadRegisters *rawFunc,
    const bool *enableFlag,
    WriteToSerial *logFunc);

#endif /* MICROLIGHT_I2C_LOG_DECORATE_H */
