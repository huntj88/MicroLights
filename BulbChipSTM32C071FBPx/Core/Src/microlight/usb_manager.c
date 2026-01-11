/*
 * usb_manager.c
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#include "microlight/usb_manager.h"
#include <stdio.h>
#include <string.h>
#include "microlight/chip_state.h"
#include "microlight/json/command_parser.h"
#include "microlight/json/json_buf.h"

// integration guide: https://github.com/hathach/tinyusb/discussions/633
bool usbInit(
    USBManager *usbManager,
    ModeManager *modeManager,
    SettingsManager *settingsManager,
    void (*enterDFU)(),
    SaveSettings saveSettings,
    SaveMode saveMode,
    UsbCdcReadTask usbCdcReadTask,
    UsbWriteToSerial usbWriteToSerial) {
    if (!usbManager || !modeManager || !settingsManager || !enterDFU || !saveSettings ||
        !saveMode || !usbCdcReadTask || !usbWriteToSerial) {
        return false;
    }
    usbManager->modeManager = modeManager;
    usbManager->settingsManager = settingsManager;
    usbManager->enterDFU = enterDFU;
    usbManager->saveSettings = saveSettings;
    usbManager->saveMode = saveMode;
    usbManager->usbCdcReadTask = usbCdcReadTask;
    usbManager->usbWriteToSerial = usbWriteToSerial;

    return true;
}

static void handleJson(USBManager *usbManager, char buffer[], size_t length) {
    parseJson(buffer, length, &cliInput);

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
            usbManager->usbWriteToSerial(errorBuf, strlen(errorBuf));
            break;
        }
        case parseWriteMode: {
            if (strcmp(cliInput.mode.name, "transientTest") == 0) {
                // do not write to flash for transient test
                setMode(usbManager->modeManager, &cliInput.mode, cliInput.modeIndex);
            } else {
                usbManager->saveMode(cliInput.modeIndex, buffer, length);
                setMode(usbManager->modeManager, &cliInput.mode, cliInput.modeIndex);
            }
            break;
        }
        case parseReadMode: {
            usbManager->modeManager->readSavedMode(
                cliInput.modeIndex, buffer, sharedJsonIOBufferSize);
            size_t len = strlen(buffer);
            if (len > sharedJsonIOBufferSize - 2) {
                len = sharedJsonIOBufferSize - 2;
            }
            buffer[len] = '\n';
            buffer[len + 1] = '\0';
            usbManager->usbWriteToSerial(buffer, len + 1);
            break;
        }
        case parseWriteSettings: {
            ChipSettings settings = cliInput.settings;
            usbManager->saveSettings(buffer, length);
            updateSettings(usbManager->settingsManager, &settings);
            break;
        }
        case parseReadSettings: {
            int len =
                getSettingsResponse(usbManager->settingsManager, buffer, sharedJsonIOBufferSize);
            usbManager->usbWriteToSerial(buffer, (size_t)len);
            break;
        }
        case parseDfu: {
            usbManager->enterDFU();
            break;
        }
    }
}

void usbTask(USBManager *usbManager) {
    int32_t bytesRead = usbManager->usbCdcReadTask(sharedJsonIOBuffer, sharedJsonIOBufferSize);
    if (bytesRead > 0) {
        handleJson(usbManager, sharedJsonIOBuffer, (size_t)bytesRead);
    }
}
