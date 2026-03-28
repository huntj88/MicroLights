/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef BOARD_API_H_
#define BOARD_API_H_

#include <stdint.h>

#include "tusb.h"

// Get board unique ID for USB serial number. Return number of bytes. Note buf_size is typically 16
size_t board_get_unique_id(uint8_t uid[], size_t buf_size);

uint32_t board_millis(void);

// Get USB Serial number string from unique ID. Return number of characters.
// Input is string descriptor from index 1 (index 0 is type + len)
static inline size_t board_usb_get_serial(uint16_t desc_str1[], size_t max_chars) {
    uint8_t uid[16] TU_ATTR_ALIGNED(4);
    size_t uid_len = board_get_unique_id(uid, sizeof(uid));

    if (uid_len > max_chars / 2) uid_len = max_chars / 2;

    for (size_t i = 0; i < uid_len; i++) {
        for (size_t j = 0; j < 2; j++) {
            const char nibble_to_hex[16] = {
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
            uint8_t const nibble = (uid[i] >> (j * 4)) & 0xf;
            desc_str1[i * 2 + (1 - j)] = nibble_to_hex[nibble];  // UTF-16-LE
        }
    }

    return 2 * uid_len;
}

#endif
