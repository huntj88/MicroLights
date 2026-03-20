/*
 * usb_dependencies.c
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#include "usb_dependencies.h"
#include "tusb.h"

void usbWrite(const char *buf, size_t count) {
    size_t sent = 0;
    uint16_t retries = 0;
    const uint16_t maxRetries = 1000;

    while (sent < count) {
        if (!tud_vendor_mounted()) {
            break;
        }

        uint32_t available = tud_vendor_write_available();
        if (available > 0) {
            uint32_t to_send = (uint32_t)(count - sent);
            if (to_send > available) {
                to_send = available;
            }

            uint32_t written = tud_vendor_write(buf + sent, to_send);
            sent += written;
            retries = 0;
        } else {
            retries++;
            if (retries >= maxRetries) {
                break;
            }
        }
        tud_task();
    }
    tud_vendor_write_flush();
    tud_task();
}

// returns bytes read if a buffer has read in a full line terminated by \n, 0 otherwise.
int32_t usbReadTask(char usbBuffer[], size_t bufferLength) {
    static uint16_t jsonIndex = 0;

    tud_task();
    if (tud_vendor_available()) {
        char buf[64];
        // cast count as uint8_t, buf is only 64 bytes
        uint8_t count = (uint8_t)tud_vendor_read(buf, sizeof(buf));
        if (jsonIndex + count > bufferLength) {
            const char *error = "{\"error\":\"payload too long\"}\n";
            usbWrite(error, strlen(error));
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
