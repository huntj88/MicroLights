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
typedef int32_t (*UsbCdcReadTask)(char *buffer, size_t length);

typedef void (*UsbWriteToSerial)(const char *buffer, size_t length);

#endif /* INC_MICROLIGHT_MODEL_USB_H_ */
