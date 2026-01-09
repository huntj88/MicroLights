/*
 * usb_manager.c
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#include "microlight/usb_manager.h"
#include <stdio.h>
#include <string.h>
#include "bootloader.h"
#include "microlight/chip_state.h"
#include "microlight/json/command_parser.h"
#include "microlight/json/json_buf.h"
#include "tusb.h"

// integration guide: https://github.com/hathach/tinyusb/discussions/633
bool usbInit(
    USBManager *usbManager,
    ModeManager *modeManager,
    SettingsManager *settingsManager,
    void (*enterDFU)(),
    SaveSettings saveSettings,
    SaveMode saveMode) {
    if (!usbManager || !modeManager || !settingsManager || !enterDFU || !saveSettings ||
        !saveMode) {
        return false;
    }
    usbManager->modeManager = modeManager;
    usbManager->settingsManager = settingsManager;
    usbManager->enterDFU = enterDFU;
    usbManager->saveSettings = saveSettings;
    usbManager->saveMode = saveMode;

    tusb_init();
    return true;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void usbWriteToSerial(USBManager *usbManager, uint8_t itf, const char *buf, uint32_t count) {
    uint32_t sent = 0;

    while (sent < count) {
        uint32_t available = tud_cdc_n_write_available(itf);
        if (available > 0) {
            uint32_t to_send = count - sent;
            if (to_send > available) {
                to_send = available;
            }

            uint32_t written = tud_cdc_n_write(itf, buf + sent, to_send);
            sent += written;
        } else {
            break;
        }
        tud_task();
    }
    tud_cdc_n_write_flush(itf);
    tud_task();
}

static void handleJson(USBManager *usbManager, char buf[], uint32_t count) {
    parseJson(buf, count, &cliInput);

    switch (cliInput.parsedType) {
        case parseError: {
            char errorBuf[256];
            if (cliInput.errorContext.error != PARSER_OK) {
                snprintf(
                    errorBuf,
                    sizeof(errorBuf),
                    "{\"error\":\"%s\",\"path\":\"%s\"}\n",
                    parserErrorToString(cliInput.errorContext.error),
                    cliInput.errorContext.path);
            } else {
                snprintf(errorBuf, sizeof(errorBuf), "{\"error\":\"unable to parse json\"}\n");
            }
            usbWriteToSerial(usbManager, 0, errorBuf, strlen(errorBuf));
            break;
        }
        case parseWriteMode: {
            if (strcmp(cliInput.mode.name, "transientTest") == 0) {
                // do not write to flash for transient test
                setMode(usbManager->modeManager, &cliInput.mode, cliInput.modeIndex);
            } else {
                usbManager->saveMode(cliInput.modeIndex, buf, count);
                setMode(usbManager->modeManager, &cliInput.mode, cliInput.modeIndex);
            }
            break;
        }
        case parseReadMode: {
            usbManager->modeManager->readSavedMode(cliInput.modeIndex, buf, JSON_BUFFER_SIZE);
            uint16_t len = strlen(buf);
            if (len > JSON_BUFFER_SIZE - 2) {
                len = JSON_BUFFER_SIZE - 2;
            }
            buf[len] = '\n';
            buf[len + 1] = '\0';
            usbWriteToSerial(usbManager, 0, buf, strlen(buf));
            break;
        }
        case parseWriteSettings: {
            ChipSettings settings = cliInput.settings;
            usbManager->saveSettings(buf, count);
            updateSettings(usbManager->settingsManager, &settings);
            break;
        }
        case parseReadSettings: {
            int len = getSettingsResponse(usbManager->settingsManager, buf, JSON_BUFFER_SIZE);
            usbWriteToSerial(usbManager, 0, buf, len);
            break;
        }
        case parseDfu: {
            usbManager->enterDFU();
            break;
        }
    }
}

void usbCdcTask(USBManager *usbManager) {
    static uint16_t jsonIndex = 0;
    uint8_t itf;

    tud_task();

    for (itf = 0; itf < CFG_TUD_CDC; itf++) {
        // connected() check for DTR bit
        // Most but not all terminal client set this when making connection
        // if ( tud_cdc_n_connected(itf) )
        {
            if (tud_cdc_n_available(itf)) {
                char buf[64];
                // cast count as uint8_t, buf is only 64 bytes
                uint8_t count = (uint8_t)tud_cdc_n_read(itf, buf, sizeof(buf));
                if (jsonIndex + count > JSON_BUFFER_SIZE) {
                    const char *error = "{\"error\":\"payload too long\"}\n";
                    usbWriteToSerial(usbManager, itf, error, strlen(error));
                    jsonIndex = 0;
                } else {
                    for (uint8_t i = 0; i < count; i++) {
                        jsonBuf[jsonIndex++] = buf[i];
                        if (buf[i] == '\n') {
                            handleJson(usbManager, jsonBuf, jsonIndex);
                            jsonIndex = 0;
                        }
                    }
                }
            }
        }
    }
}
