/*
 * usb_dependencies.c
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#include "usb_dependencies.h"
#include "tusb.h"

void usbWriteToSerial(const char *buf, int count) {
    int sent = 0;
    uint8_t itf = 0;

    while (sent < count) {
        uint32_t available = tud_cdc_n_write_available(itf);
        if (available > 0) {
            uint32_t to_send = count - sent;
            if (to_send > available) {
                to_send = available;
            }

            uint32_t written = tud_cdc_n_write(itf, buf + sent, to_send);
            sent += written;
        } else {
            break;
        }
        tud_task();
    }
    tud_cdc_n_write_flush(itf);
    tud_task();
}

// returns bytes read if a buffer has read in a full line terminated by \n, 0 otherwise.
int usbCdcReadTask(char usbBuffer[], int bufferLength) {
    static uint16_t jsonIndex = 0;
    uint8_t itf = 0;

    tud_task();
    if (tud_cdc_n_available(itf)) {
        char buf[64];
        // cast count as uint8_t, buf is only 64 bytes
        uint8_t count = (uint8_t)tud_cdc_n_read(itf, buf, sizeof(buf));
        if (jsonIndex + count > bufferLength) {
            const char *error = "{\"error\":\"payload too long\"}\n";
            usbWriteToSerial(error, strlen(error));
            jsonIndex = 0;
        } else {
            for (uint8_t i = 0; i < count; i++) {
                usbBuffer[jsonIndex++] = buf[i];
                if (buf[i] == '\n') {
                    int len = jsonIndex;
                    jsonIndex = 0;
                    return len;
                }
            }
        }
    }

    return 0;
}
