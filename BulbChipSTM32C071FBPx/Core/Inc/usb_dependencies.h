/*
 * usb_dependencies.h
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#ifndef INC_USB_DEPENDENCIES_H_
#define INC_USB_DEPENDENCIES_H_

#include <stdbool.h>
#include <stdint.h>

void usbWriteToSerial(const char *buf, int count);

// returns bytes read if a buffer has read in a full line terminated by \n, 0 otherwise.
int usbCdcReadTask(char usbBuffer[], int bufferLength);

#endif /* INC_USB_DEPENDENCIES_H_ */
