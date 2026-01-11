/*
 * settings_manager.c
 *
 *  Created on: Dec 18, 2025
 *      Author: jameshunt
 */

#include "microlight/settings_manager.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "microlight/json/command_parser.h"
#include "microlight/json/json_buf.h"

static void loadSettingsFromFlash(SettingsManager *manager, char *buffer, CliInput *cliInput);

// TODO: const pointers?
bool settingsManagerInit(SettingsManager *manager, ReadSavedSettings readSavedSettings) {
    if (!manager || !readSavedSettings) {
        return false;
    }
    manager->readSavedSettings = readSavedSettings;

    loadSettingsFromFlash(manager, sharedJsonIOBuffer, &cliInput);
    return true;
}

static void loadSettingsFromFlash(SettingsManager *manager, char *buffer, CliInput *cliInput) {
    // Set defaults first in case load fails
    chipSettingsInitDefaults(&manager->currentSettings);

    manager->readSavedSettings(buffer, sharedJsonIOBufferSize);

    parseJson(buffer, sharedJsonIOBufferSize, cliInput);

    if (cliInput->parsedType == parseWriteSettings) {
        updateSettings(manager, &cliInput->settings);
    }
}

void updateSettings(SettingsManager *manager, ChipSettings *newSettings) {
    manager->currentSettings = *newSettings;
}

static int appendJson(char *buffer, size_t length, int offset, const char *format, ...) {
    if (offset >= (int)length) {
        return offset;
    }

    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer + offset, length - offset, format, args);
    va_end(args);

    if (written > 0) {
        offset += written;
    }
    return offset;
}

// Helper macros for printing
#define PRINT_VAL_uint8_t(val) "%d", (int)(val)
#define PRINT_VAL_bool(val) "%s", (val) ? "true" : "false"

int getSettingsDefaultsJson(char *buffer, size_t length) {
    ChipSettings settings;
    chipSettingsInitDefaults(&settings);

    int offset = 0;

    offset = appendJson(buffer, length, offset, "{");

    bool first = true;
#define X_PRINT(type, name, def)                                                  \
    if (!first) {                                                                 \
        offset = appendJson(buffer, length, offset, ",");                         \
    }                                                                             \
    offset = appendJson(buffer, length, offset, "\"" #name "\":");                \
    offset = appendJson(buffer, length, offset, PRINT_VAL_##type(settings.name)); \
    first = false;

    CHIP_SETTINGS_MAP(X_PRINT)
#undef X_PRINT

    offset = appendJson(buffer, length, offset, "}");

    return offset;
}

int getSettingsResponse(SettingsManager *manager, char *buffer, size_t length) {
    loadSettingsFromFlash(manager, buffer, &cliInput);
    bool hasSettings = (cliInput.parsedType == parseWriteSettings);

    int offset = 0;
    if (hasSettings) {
        size_t currentLen = strlen(buffer);
        const char *prefix = "{\"settings\":";
        size_t prefixLen = strlen(prefix);

        if (prefixLen + currentLen >= length) {
            return 0;
        }

        // Shift existing data to make room for prefix
        memmove(buffer + prefixLen, buffer, currentLen + 1);

        // prepend prefix, no null terminator needed
        // NOLINTNEXTLINE(bugprone-not-null-terminated-result)
        memcpy(buffer, prefix, prefixLen);

        offset = (int)(prefixLen + currentLen);
    } else {
        offset = appendJson(buffer, length, offset, "{\"settings\":null");
    }

    char defaultsBuf[SETTINGS_DEFAULTS_JSON_SIZE];
    getSettingsDefaultsJson(defaultsBuf, sizeof(defaultsBuf));

    offset = appendJson(buffer, length, offset, ",\"defaults\":%s", defaultsBuf);
    offset = appendJson(buffer, length, offset, "}\n");
    return offset;
}
