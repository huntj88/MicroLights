/*
 * json_manager.h
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#ifndef INC_USB_MANAGER_H_
#define INC_USB_MANAGER_H_

#include <stdint.h>
#include "mode_manager.h"
#include "settings_manager.h"
#include "model/serial.h"
#include "main.h"

typedef struct USBManager {
	UART_HandleTypeDef *huart;
	ModeManager *modeManager;
	SettingsManager *settingsManager;
	void (*enterDFU)();
} USBManager;

void usbInit(
	USBManager *usbManager,
	UART_HandleTypeDef *huart,
	ModeManager *mm,
	SettingsManager *sm,
	void (*enterDFU)()
);

void usbCdcTask(USBManager *usbManager);
void usbWriteToSerial(USBManager *usbManager, uint8_t itf, const char *buf, uint32_t count);

#endif /* INC_USB_MANAGER_H_ */
