/*
 * json_buf.c
 *
 *  Created on: Dec 31, 2025
 *      Author: jameshunt
 */

#include "microlight/json/json_buf.h"

#include <stddef.h>

/* Global buffer for reading JSON data.
 * Note: This variable is not thread-safe and is intended for single-threaded use only.
 */
char *sharedJsonIOBuffer = NULL;
size_t sharedJsonIOBufferLength = 0;

bool initSharedJsonIOBuffer(char *buffer, size_t length) {
    if (!buffer || length == 0) {
        return false;
    }
    sharedJsonIOBuffer = buffer;
    sharedJsonIOBufferLength = length;
    return true;
}
