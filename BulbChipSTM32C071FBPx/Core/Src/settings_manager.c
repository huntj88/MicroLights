/*
 * settings_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "settings_manager.h"
#include <string.h>
#include "json/command_parser.h"
#include "json/json_buf.h"
#include "storage.h"

bool settingsManagerInit(
    SettingsManager *manager, void (*readSettingsFromFlash)(char *buffer, uint32_t length)) {
    if (!manager || !readSettingsFromFlash) {
        return false;
    }
    manager->readSettingsFromFlash = readSettingsFromFlash;

    loadSettingsFromFlash(manager, jsonBuf);
    return true;
}

void loadSettingsFromFlash(SettingsManager *manager, char *buffer) {
    // Set defaults first in case load fails
    chipSettingsInitDefaults(&manager->currentSettings);

    manager->readSettingsFromFlash(buffer, PAGE_SECTOR);

    parseJson((uint8_t *)buffer, PAGE_SECTOR, &cliInput);

    if (cliInput.parsedType == parseWriteSettings) {
        updateSettings(manager, &cliInput.settings);
    }
}

void updateSettings(SettingsManager *manager, ChipSettings *newSettings) {
    manager->currentSettings = *newSettings;
}
