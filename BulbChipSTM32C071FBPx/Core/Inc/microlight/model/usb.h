/*
 * usb.h
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#ifndef INC_MICROLIGHT_MODEL_USB_H_
#define INC_MICROLIGHT_MODEL_USB_H_

#include <stddef.h>
#include <stdint.h>

// Returns bytes read
typedef int32_t (*UsbCdcReadTask)(char usbBuffer[], size_t bufferLength);

typedef void (*UsbWriteToSerial)(const char usbBuffer[], size_t bufferLength);

#endif /* INC_MICROLIGHT_MODEL_USB_H_ */
