/*
 * mcu_dependencies.c
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include "mcu_dependencies.h"
#include "storage.h"

void writeSettingsToFlash(const char str[], uint32_t length) {
    writeStringToFlash(SETTINGS_PAGE, str, length);
}

void readSettingsFromFlash(char buffer[], uint32_t length) {
    readStringFromFlash(SETTINGS_PAGE, buffer, length);
}

void writeBulbModeToFlash(uint8_t mode, const char str[], uint32_t length) {
    uint32_t page = BULB_PAGE_0 + mode;
    writeStringToFlash(page, str, length);
}

void readBulbModeFromFlash(uint8_t mode, char buffer[], uint32_t length) {
    uint32_t page = BULB_PAGE_0 + mode;
    readStringFromFlash(page, buffer, length);
}
