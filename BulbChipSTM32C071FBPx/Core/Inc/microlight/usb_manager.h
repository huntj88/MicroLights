/*
 * usb_manager.h
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#ifndef INC_USB_MANAGER_H_
#define INC_USB_MANAGER_H_

#include <stdbool.h>
#include <stdint.h>
#include "mode_manager.h"
#include "model/log.h"
#include "model/storage.h"
#include "model/usb.h"
#include "settings_manager.h"

typedef struct USBManager {
    ModeManager *modeManager;
    SettingsManager *settingsManager;
    void (*enterDFU)();
    SaveSettings saveSettings;
    SaveMode saveMode;
    UsbCdcReadTask usbCdcReadTask;
    UsbWriteToSerial usbWriteToSerial;
} USBManager;

bool usbInit(
    USBManager *usbManager,
    ModeManager *modeManager,
    SettingsManager *settingsManager,
    void (*enterDFU)(),
    SaveSettings saveSettings,
    SaveMode saveMode,
    UsbCdcReadTask usbCdcReadTask,
    UsbWriteToSerial usbWriteToSerial);

void usbTask(USBManager *usbManager);

#endif /* INC_USB_MANAGER_H_ */
