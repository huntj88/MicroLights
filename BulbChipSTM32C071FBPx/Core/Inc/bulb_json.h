/*
 * bulb_json.h
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */

#ifndef INC_BULB_JSON_H_
#define INC_BULB_JSON_H_

#include <stdint.h>

enum Output {
	low, high
};

typedef struct ChangeAt {
	uint16_t tick;
	enum Output output;
} ChangeAt;

typedef struct BulbMode {
	char name[32];
	uint8_t modeIndex;
	uint16_t totalTicks;
	ChangeAt changeAt[64];
	uint8_t numChanges;
	uint16_t jsonLength;
} BulbMode;

void parseJson(uint8_t buf[], uint32_t count, BulbMode *mode);

#endif /* INC_BULB_JSON_H_ */
