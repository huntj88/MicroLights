/*
 * json_buf.h
 *
 *  Created on: Dec 31, 2025
 *      Author: jameshunt
 */

#ifndef INC_JSON_JSON_BUF_H_
#define INC_JSON_JSON_BUF_H_

#include <stdint.h>

extern char *sharedJsonIOBuffer;
extern uint32_t sharedJsonIOBufferSize;

void initSharedJsonIOBuffer(char *buf, uint32_t size);

#endif /* INC_JSON_JSON_BUF_H_ */
