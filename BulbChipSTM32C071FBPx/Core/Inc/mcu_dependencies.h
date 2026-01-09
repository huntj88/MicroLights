/*
 * mcu_dependencies.h
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include <stdint.h>

#ifndef INC_MCU_DEPENDENCIES_H_
#define INC_MCU_DEPENDENCIES_H_

#define SETTINGS_PAGE 56       // 2K flash reserved for settings starting at page 56
#define BULB_PAGE_0 57         // 14K flash reserved for bulb modes starting at page 57

void writeSettingsToFlash(const char str[], uint32_t length);
void readSettingsFromFlash(char buffer[], uint32_t length);

void writeBulbModeToFlash(uint8_t mode, const char str[], uint32_t length);
void readBulbModeFromFlash(uint8_t mode, char buffer[], uint32_t length);

#endif /* INC_MCU_DEPENDENCIES_H_ */
