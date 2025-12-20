/*
 * json_manager.h
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#ifndef INC_JSON_USB_COMMAND_MANAGER_H_
#define INC_JSON_USB_COMMAND_MANAGER_H_

#include <stdint.h>
#include "mode_manager.h"
#include "settings_manager.h"
#include "model/serial.h"

void handleJson(
    ModeManager *modeManager,
    SettingsManager *settingsManager,
    WriteToUsbSerial *writeUsbSerial,
    void (*enterDFU)(),
    uint8_t buf[],
    uint32_t count
);

#endif /* INC_JSON_USB_COMMAND_MANAGER_H_ */
