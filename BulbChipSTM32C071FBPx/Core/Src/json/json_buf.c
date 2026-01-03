/*
 * json_buf.c
 *
 *  Created on: Dec 31, 2025
 *      Author: jameshunt
 */

#include "json/json_buf.h"

/* Global buffer for reading JSON data.
 * Note: This variable is not thread-safe and is intended for single-threaded use only.
 */
char jsonBuf[PAGE_SECTOR];
