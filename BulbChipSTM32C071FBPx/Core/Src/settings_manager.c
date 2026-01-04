/*
 * settings_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "settings_manager.h"
#include <stdarg.h>
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

static int appendJson(char *buffer, uint32_t len, int offset, const char *format, ...) {
    if (offset >= (int)len) {
        return offset;
    }

    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + offset, len - offset, format, args);
    va_end(args);

    if (written > 0) {
        offset += written;
    }
    return offset;
}

// Helper macros for printing
#define PRINT_VAL_uint8_t(val) "%d", (int)(val)
#define PRINT_VAL_bool(val) "%s", (val) ? "true" : "false"

int getSettingsDefaultsJson(char *buffer, uint32_t len) {
    ChipSettings settings;
    chipSettingsInitDefaults(&settings);

    int offset = 0;

    offset = appendJson(buffer, len, offset, ",\"defaults\":{");

    bool first = true;
#define X_PRINT(type, name, def)                                               \
    if (!first) {                                                              \
        offset = appendJson(buffer, len, offset, ",");                         \
    }                                                                          \
    offset = appendJson(buffer, len, offset, "\"" #name "\":");                \
    offset = appendJson(buffer, len, offset, PRINT_VAL_##type(settings.name)); \
    first = false;

    CHIP_SETTINGS_MAP(X_PRINT)
#undef X_PRINT

    offset = appendJson(buffer, len, offset, "}");

    return offset;
}

int getSettingsResponse(char *buffer, uint32_t len, const char *currentSettingsJson) {
    int offset = 0;
    offset = appendJson(buffer, len, offset, "{\"settings\":");

    if (currentSettingsJson) {
        offset = appendJson(buffer, len, offset, "%s", currentSettingsJson);
    } else {
        offset = appendJson(buffer, len, offset, "null");
    }

    char defaultsBuf[256];
    getSettingsDefaultsJson(defaultsBuf, sizeof(defaultsBuf));

    offset = appendJson(buffer, len, offset, "%s", defaultsBuf);
    offset = appendJson(buffer, len, offset, "}\n");

    return offset;
}
