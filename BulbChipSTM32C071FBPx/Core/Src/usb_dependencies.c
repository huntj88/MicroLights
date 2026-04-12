/*
 * usb_dependencies.c
 *
 *  Created on: Jan 9, 2026
 *      Author: jameshunt
 */

#include "usb_dependencies.h"
#include "bsp/board_api.h"
#include "tusb.h"

// File-scope state for usbReadTask buffering
static uint16_t jsonIndex = 0;
static char readBuf[64];
static uint8_t readBufCount = 0;
static uint8_t readBufPos = 0;

void usbWrite(const char *buf, size_t count) {
    size_t sent = 0;
    // Large payloads can span many 64-byte packets. Give the host
    // enough time to drain the vendor TX FIFO instead of truncating the tail.
    const uint32_t writeTimeoutMs = 1000;
    uint32_t lastProgressAt = board_millis();

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
            if (written > 0) {
                lastProgressAt = board_millis();
                if (sent < count) {
                    tud_vendor_write_flush();
                }
            }
        } else {
            tud_vendor_write_flush();
            if (board_millis() - lastProgressAt >= writeTimeoutMs) {
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
        char chr = readBuf[readBufPos++];
        usbBuffer[jsonIndex++] = chr;
        if (chr == '\n') {
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
                char chr = readBuf[readBufPos++];
                usbBuffer[jsonIndex++] = chr;
                if (chr == '\n') {
                    int32_t len = (int32_t)jsonIndex;
                    jsonIndex = 0;
                    return len;
                }
            }
        }
    }

    return 0;
}
