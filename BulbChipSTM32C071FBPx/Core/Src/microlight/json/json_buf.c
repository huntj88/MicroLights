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
size_t sharedJsonIOBufferSize = 0;

bool initSharedJsonIOBuffer(char *buf, size_t size) {
    if (buf == NULL || size == 0) {
        return false;
    }
    sharedJsonIOBuffer = buf;
    sharedJsonIOBufferSize = size;
    return true;
}
