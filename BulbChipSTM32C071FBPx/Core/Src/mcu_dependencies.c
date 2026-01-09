/*
 * mcu_dependencies.c
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include "mcu_dependencies.h"
#include "storage.h"

#define SETTINGS_PAGE 56  // 2K flash reserved for settings starting at page 56
#define BULB_PAGE_0 57    // 14K flash reserved for bulb modes starting at page 57

void writeSettingsToFlash(const char str[], uint32_t length) {
    writeStringToFlash(SETTINGS_PAGE, str, length);
}

void readSettingsFromFlash(char buffer[], uint32_t length) {
    readStringFromFlash(SETTINGS_PAGE, buffer, length);
}

void writeModeToFlash(uint8_t mode, const char str[], uint32_t length) {
    uint32_t page = BULB_PAGE_0 + mode;
    writeStringToFlash(page, str, length);
}

void readModeFromFlash(uint8_t mode, char buffer[], uint32_t length) {
    uint32_t page = BULB_PAGE_0 + mode;
    readStringFromFlash(page, buffer, length);
}
