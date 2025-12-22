/*
 * settings_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "settings_manager.h"
#include <string.h>
#include "json/command_parser.h"
#include "storage.h"

bool settingsManagerInit(SettingsManager *manager,
                         void (*readSettingsFromFlash)(char *buffer, uint32_t length)) {
    if (!manager || !readSettingsFromFlash) {
        return false;
    }
    manager->readSettingsFromFlash = readSettingsFromFlash;
    settingsManagerLoad(manager);
    return true;
}

void settingsManagerLoadFromBuffer(SettingsManager *manager, char *buffer) {
    // Set defaults first in case load fails
    manager->currentSettings.modeCount = 0;
    manager->currentSettings.minutesUntilAutoOff = 90;
    manager->currentSettings.minutesUntilLockAfterAutoOff = 10;

    manager->readSettingsFromFlash(buffer, 1024);
    parseJson((uint8_t *)buffer, 1024, &cliInput);

    if (cliInput.parsedType == parseWriteSettings) {
        manager->currentSettings = cliInput.settings;
    }
}

void settingsManagerLoad(SettingsManager *manager) {
    char flashReadBuffer[1024];
    settingsManagerLoadFromBuffer(manager, flashReadBuffer);
}

void settingsManagerUpdate(SettingsManager *manager, ChipSettings *newSettings) {
    manager->currentSettings = *newSettings;
}
