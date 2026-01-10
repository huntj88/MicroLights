/*
 * storage.h
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#ifndef INC_MODEL_STORAGE_H_
#define INC_MODEL_STORAGE_H_

#include <stddef.h>
#include <stdint.h>

typedef void (*SaveSettings)(const char str[], size_t length);
typedef void (*ReadSavedSettings)(char buffer[], size_t length);

typedef void (*SaveMode)(uint8_t mode, const char str[], size_t length);
typedef void (*ReadSavedMode)(uint8_t mode, char buffer[], size_t length);

#endif /* INC_MODEL_STORAGE_H_ */
