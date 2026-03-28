/*
 * usb_dependencies.c
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#include "usb_dependencies.h"
#include "tusb.h"

// File-scope state for usbReadTask buffering
static uint16_t jsonIndex = 0;
static char readBuf[64];
static uint8_t readBufCount = 0;
static uint8_t readBufPos = 0;

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

#ifdef UNIT_TEST
void usbReadTaskReset(void) {
    jsonIndex = 0;
    readBufCount = 0;
    readBufPos = 0;
}
#endif

// returns bytes read if a buffer has read in a full line terminated by \n, 0 otherwise.
int32_t usbReadTask(char usbBuffer[], size_t bufferLength) {
    // Process any leftover bytes from a previous read before pulling new data
    while (readBufPos < readBufCount) {
        if (jsonIndex + 1 > bufferLength) {
            const char *error = "{\"error\":\"payload too long\"}\n";
            usbWrite(error, strlen(error));
            jsonIndex = 0;
            readBufPos = 0;
            readBufCount = 0;
            return 0;
        }
        char c = readBuf[readBufPos++];
        usbBuffer[jsonIndex++] = c;
        if (c == '\n') {
            int32_t len = (int32_t)jsonIndex;
            jsonIndex = 0;
            return len;
        }
    }

    // All leftover bytes consumed, try to read new data
    tud_task();
    if (tud_vendor_available()) {
        // cast count as uint8_t, readBuf is only 64 bytes
        readBufCount = (uint8_t)tud_vendor_read(readBuf, sizeof(readBuf));
        readBufPos = 0;

        if (jsonIndex + readBufCount > bufferLength) {
            const char *error = "{\"error\":\"payload too long\"}\n";
            usbWrite(error, strlen(error));
            jsonIndex = 0;
            readBufPos = 0;
            readBufCount = 0;
        } else {
            while (readBufPos < readBufCount) {
                char c = readBuf[readBufPos++];
                usbBuffer[jsonIndex++] = c;
                if (c == '\n') {
                    int32_t len = (int32_t)jsonIndex;
                    jsonIndex = 0;
                    return len;
                }
            }
        }
    }

    return 0;
}
