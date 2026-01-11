/*
 * usb_dependencies.h
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#ifndef INC_USB_DEPENDENCIES_H_
#define INC_USB_DEPENDENCIES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void usbWriteToSerial(const char *buf, size_t count);

// returns bytes read if a buffer has read in a full line terminated by \n, 0 otherwise.
int32_t usbCdcReadTask(char usbBuffer[], size_t bufferLength);

#endif /* INC_USB_DEPENDENCIES_H_ */
