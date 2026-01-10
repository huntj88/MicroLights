/*
 * i2c.h
 *
 *  Created on: Dec 23, 2025
 *      Author: jameshunt
 */

#ifndef INC_DEVICE_I2C_H_
#define INC_DEVICE_I2C_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// write to single register.
typedef void (*I2CWriteRegister)(uint8_t devAddress, uint8_t reg, uint8_t value);

// write to single register. Returns true on success.
typedef bool (*I2CWriteRegisterChecked)(uint8_t devAddress, uint8_t reg, uint8_t value);

// Read multiple consecutive registers. Returns true on success.
typedef bool (*I2CReadRegisters)(
    uint8_t devAddress, uint8_t startReg, uint8_t *buffer, size_t length);

#endif /* INC_DEVICE_I2C_H_ */
