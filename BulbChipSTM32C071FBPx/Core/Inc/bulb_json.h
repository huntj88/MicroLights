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

enum ParseResult {
	parseError,
	parseWriteMode,
	parseReadMode,
	parseWriteSettings,
	parseReadSettings,
	parseDfu
};

typedef struct ChangeAt {
	uint16_t tick;
	enum Output output;
} ChangeAt;

typedef struct Waveform {
	char name[32];
	uint16_t totalTicks;
	ChangeAt changeAt[64];
	uint8_t numChanges;
} Waveform;

typedef struct AccelTrigger {
	uint8_t threshold;
	char color[8]; // e.g. "#3584e4"
	Waveform waveform;

	// metadata calculated at runtime
	int8_t red;
	uint8_t green;
	uint8_t blue;
} AccelTrigger;

typedef struct BulbMode {
	char name[32];
	char color[8]; // e.g. "#3584e4"
	Waveform waveform;
	AccelTrigger triggers[8];

	// metadata calculated at runtime
	uint8_t triggerCount;
	uint8_t modeIndex;
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} BulbMode;

typedef struct ChipSettings {
	uint8_t modeCount;
	uint8_t minutesUntilAutoOff;
	uint8_t minutesUntilLockAfterAutoOff;
	// TODO: uint16_t chipTickPeriodMillis;
} ChipSettings;

typedef struct CliInput {
	// only one will be populated, see parsedType
	BulbMode mode;
	ChipSettings settings;

	// metadata calculated at runtime
	uint16_t jsonLength;
	// 0 for not parsed successfully
	// 1 for mode
	// 2 for settings
	enum ParseResult parsedType;
} CliInput;

void parseJson(uint8_t buf[], uint32_t count, CliInput *input);

#endif /* INC_BULB_JSON_H_ */
