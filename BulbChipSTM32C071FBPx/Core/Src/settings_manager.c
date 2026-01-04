/*
 * settings_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "settings_manager.h"
#include <stdio.h>
#include <string.h>
#include "json/command_parser.h"
#include "json/json_buf.h"
#include "storage.h"

bool settingsManagerInit(
    SettingsManager *manager, void (*readSettingsFromFlash)(char buffer[], uint32_t length)) {
    if (!manager || !readSettingsFromFlash) {
        return false;
    }
    manager->readSettingsFromFlash = readSettingsFromFlash;

    loadSettingsFromFlash(manager, jsonBuf);
    return true;
}

void loadSettingsFromFlash(SettingsManager *manager, char buffer[]) {
    // Set defaults first in case load fails
    chipSettingsInitDefaults(&manager->currentSettings);

    manager->readSettingsFromFlash(buffer, PAGE_SECTOR);

    parseJson(buffer, PAGE_SECTOR, &cliInput);

    if (cliInput.parsedType == parseWriteSettings) {
        updateSettings(manager, &cliInput.settings);
    }
}

void updateSettings(SettingsManager *manager, ChipSettings *newSettings) {
    manager->currentSettings = *newSettings;
}

// Helper macros for printing
#define PRINT_VAL_uint8_t(val) "%d", (int)(val)
#define PRINT_VAL_bool(val) "%s", (val) ? "true" : "false"

int getSettingsDefaultsJson(char *buffer, uint32_t len) {
    ChipSettings s;
    chipSettingsInitDefaults(&s);

    int offset = 0;
    // Use a safe snprintf wrapper or check bounds
    #define SAFE_PRINT(...) do { \
        if (offset < len) offset += snprintf(buffer + offset, len - offset, __VA_ARGS__); \
    } while(0)

    SAFE_PRINT(",\"defaults\":{");

    bool first = true;
    #define X_PRINT(type, name, def) \
        if (!first) SAFE_PRINT(","); \
        SAFE_PRINT("\"" #name "\":"); \
        SAFE_PRINT(PRINT_VAL_##type(s.name)); \
        first = false;

    CHIP_SETTINGS_MAP(X_PRINT)
    #undef X_PRINT

    SAFE_PRINT("}}\n");
    
    return offset;
}
