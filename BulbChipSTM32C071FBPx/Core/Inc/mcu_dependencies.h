/*
 * mcu_dependencies.h
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef INC_MCU_DEPENDENCIES_H_
#define INC_MCU_DEPENDENCIES_H_

void writeSettingsToFlash(const char str[], uint32_t length);
void readSettingsFromFlash(char buffer[], uint32_t length);

void writeModeToFlash(uint8_t mode, const char str[], uint32_t length);
void readModeFromFlash(uint8_t mode, char buffer[], uint32_t length);

bool i2cReadRegister(uint8_t devAddress, uint8_t reg, uint8_t *data);
bool i2cWriteRegister(uint8_t devAddress, uint8_t reg, uint8_t value);
bool i2cReadRegisters(uint8_t devAddress, uint8_t startReg, uint8_t *buf, size_t len);

uint8_t readButtonPin(void);
void writeRgbPwmCaseLed(uint16_t redDuty, uint16_t greenDuty, uint16_t blueDuty);
void writeBulbLed(uint8_t state);

void enableTimers(bool enable);
uint32_t convertTicksToMilliseconds(uint32_t ticks);

#endif /* INC_MCU_DEPENDENCIES_H_ */
