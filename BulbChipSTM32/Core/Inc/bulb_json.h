/*
 * bulb_json.h
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */

#ifndef INC_BULB_JSON_H_
#define INC_BULB_JSON_H_

enum Output {
	low, high
};

typedef struct ChangeAt {
	uint16_t tick;
	enum Output output;
} ChangeAt;

typedef struct BulbMode {
	char name[32];
	uint16_t totalTicks;
	ChangeAt changeAt[64];
	uint8_t numChanges;
} BulbMode;

BulbMode parseJson(uint8_t buf[], uint32_t count);

#endif /* INC_BULB_JSON_H_ */
