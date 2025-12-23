/*
 * usb_manager.c
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#include <stdio.h>
#include <string.h>
#include <usb_manager.h>
#include "bootloader.h"
#include "chip_state.h"
#include "json/command_parser.h"
#include "storage.h"
#include "tusb.h"

// integration guide: https://github.com/hathach/tinyusb/discussions/633
bool usbInit(
    USBManager *usbManager,
    UART_HandleTypeDef *huart,
    ModeManager *_modeManager,
    SettingsManager *_settingsManager,
    void (*_enterDFU)()) {
    if (!usbManager || !huart || !_modeManager || !_settingsManager || !_enterDFU) {
        return false;
    }
    usbManager->huart = huart;
    usbManager->modeManager = _modeManager;
    usbManager->settingsManager = _settingsManager;
    usbManager->enterDFU = _enterDFU;

    tusb_init();
    return true;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void usbWriteToSerial(USBManager *usbManager, uint8_t itf, const char *buf, uint32_t count) {
    for (uint32_t i = 0; i < count; i += 64) {
        uint32_t countMax64 = MIN(count - i, 64);
        tud_cdc_n_write(itf, buf + i, countMax64);
        tud_task();
    }
    tud_cdc_n_write_flush(itf);
    tud_task();

    // also log to uart serial in case usb doesn't work
    if (usbManager->huart != NULL) {
        HAL_UART_Transmit(usbManager->huart, (uint8_t *)buf, count, 100);
    }
}

static void handleJson(USBManager *usbManager, uint8_t buf[], uint32_t count) {
    parseJson(buf, count, &cliInput);

    switch (cliInput.parsedType) {
        case parseError: {
            char errorBuf[256];
            if (cliInput.errorContext.error != MODE_PARSER_OK) {
                snprintf(
                    errorBuf,
                    sizeof(errorBuf),
                    "{\"error\":\"%s\",\"path\":\"%s\"}\n",
                    modeParserErrorToString(cliInput.errorContext.error),
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
                writeBulbModeToFlash(cliInput.modeIndex, buf, cliInput.jsonLength);
                setMode(usbManager->modeManager, &cliInput.mode, cliInput.modeIndex);
            }
            break;
        }
        case parseReadMode: {
            char flashReadBuffer[1024];
            loadModeFromBuffer(usbManager->modeManager, cliInput.modeIndex, flashReadBuffer);
            uint16_t len = strlen(flashReadBuffer);
            flashReadBuffer[len] = '\n';
            flashReadBuffer[len + 1] = '\0';
            usbWriteToSerial(usbManager, 0, flashReadBuffer, strlen(flashReadBuffer));
            break;
        }
        case parseWriteSettings: {
            ChipSettings settings = cliInput.settings;
            writeSettingsToFlash(buf, cliInput.jsonLength);
            settingsManagerUpdate(usbManager->settingsManager, &settings);
            break;
        }
        case parseReadSettings: {
            char flashReadBuffer[1024];
            settingsManagerLoadFromBuffer(usbManager->settingsManager, flashReadBuffer);
            uint16_t len = strlen(flashReadBuffer);
            flashReadBuffer[len] = '\n';
            flashReadBuffer[len + 1] = '\0';
            usbWriteToSerial(usbManager, 0, flashReadBuffer, strlen(flashReadBuffer));
            break;
        }
        case parseDfu: {
            usbManager->enterDFU();
            break;
        }
    }
}

void usbCdcTask(USBManager *usbManager) {
    static uint8_t jsonBuf[1024];
    static uint16_t jsonIndex = 0;
    uint8_t itf;

    tud_task();

    for (itf = 0; itf < CFG_TUD_CDC; itf++) {
        // connected() check for DTR bit
        // Most but not all terminal client set this when making connection
        // if ( tud_cdc_n_connected(itf) )
        {
            if (tud_cdc_n_available(itf)) {
                uint8_t buf[64];
                // cast count as uint8_t, buf is only 64 bytes
                uint8_t count = (uint8_t)tud_cdc_n_read(itf, buf, sizeof(buf));
                if (jsonIndex + count > sizeof(jsonBuf)) {
                    char error[] = "{\"error\":\"payload too long\"}\n";
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
