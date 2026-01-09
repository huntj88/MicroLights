/*
 * mcu_dependencies.h
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include <stdint.h>

#ifndef INC_MCU_DEPENDENCIES_H_
#define INC_MCU_DEPENDENCIES_H_

void writeSettingsToFlash(const char str[], uint32_t length);
void readSettingsFromFlash(char buffer[], uint32_t length);

void writeModeToFlash(uint8_t mode, const char str[], uint32_t length);
void readModeFromFlash(uint8_t mode, char buffer[], uint32_t length);
#endif /* INC_MCU_DEPENDENCIES_H_ */
