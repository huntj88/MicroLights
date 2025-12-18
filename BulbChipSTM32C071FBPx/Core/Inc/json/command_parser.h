/*
 * bulb_json.h
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */

#ifndef INC_COMMAND_PARSER_H_
#define INC_COMMAND_PARSER_H_

#include <stdint.h>
#include "mode_parser.h"

/*
 * Example Commands:
 *
 * Write Mode:
 * {
 *   "command": "writeMode",
 *   "index": 0,
 *   "mode": { ... } // Refer to mode parser for the full json object
 * }
 *
 * Read Mode:
 * {
 *   "command": "readMode",
 *   "index": 0
 * }
 *
 * Write Settings:
 * {
 *   "command": "writeSettings",
 *   "modeCount": 5,
 *   "minutesUntilAutoOff": 30,
 *   "minutesUntilLockAfterAutoOff": 60
 * }
 *
 * Read Settings:
 * {
 *   "command": "readSettings"
 * }
 *
 * DFU:
 * {
 *   "command": "dfu"
 * }
 */

enum ParseResult {
	parseError,
	parseWriteMode,
	parseReadMode,
	parseWriteSettings,
	parseReadSettings,
	parseDfu
};

typedef struct ChipSettings {
	uint8_t modeCount;
	uint8_t minutesUntilAutoOff;
	uint8_t minutesUntilLockAfterAutoOff;
} ChipSettings;

typedef struct CliInput {
	// only one will be populated, see parsedType
	Mode mode;
	ChipSettings settings;

	// metadata
	uint8_t modeIndex;

	// metadata calculated at runtime
	uint16_t jsonLength;
	
	// metadata calculated at runtime
	// 0 for not parsed successfully
	// 1 for mode
	// 2 for settings
	enum ParseResult parsedType;
} CliInput;

void parseJson(uint8_t buf[], uint32_t count, CliInput *input);

#endif /* INC_COMMAND_PARSER_H_ */
