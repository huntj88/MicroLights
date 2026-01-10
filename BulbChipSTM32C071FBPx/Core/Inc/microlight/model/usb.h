/*
 * usb.h
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#ifndef INC_MICROLIGHT_MODEL_USB_H_
#define INC_MICROLIGHT_MODEL_USB_H_

#include <stdint.h>

// Returns bytes read
typedef int (*UsbCdcReadTask)(char usbBuffer[], int bufferLength);

typedef void (*UsbWriteToSerial)(const char usbBuffer[], int bufferLength);

#endif /* INC_MICROLIGHT_MODEL_USB_H_ */
