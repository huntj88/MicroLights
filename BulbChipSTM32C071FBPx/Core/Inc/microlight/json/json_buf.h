/*
 * json_buf.h
 *
 *  Created on: Dec 31, 2025
 *      Author: jameshunt
 */

#ifndef INC_JSON_JSON_BUF_H_
#define INC_JSON_JSON_BUF_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern char *sharedJsonIOBuffer;
extern size_t sharedJsonIOBufferSize;

bool initSharedJsonIOBuffer(char *buf, size_t size);

#endif /* INC_JSON_JSON_BUF_H_ */
