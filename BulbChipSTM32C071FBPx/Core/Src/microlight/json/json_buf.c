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
uint32_t sharedJsonIOBufferSize = 0;

void initSharedJsonIOBuffer(char *buf, uint32_t size) {
    sharedJsonIOBuffer = buf;
    sharedJsonIOBufferSize = size;
}
