/*
 * cli_model.h
 *
 *  Created on: Dec 19, 2025
 *      Author: jameshunt
 */

#ifndef INC_MODEL_CLI_MODEL_H_
#define INC_MODEL_CLI_MODEL_H_

#include <stdint.h>
#include "model/mode.h"
#include "model/chip_settings.h"
#include "json/mode_parser.h"

enum ParseResult {
	parseError,
	parseWriteMode,
	parseReadMode,
	parseWriteSettings,
	parseReadSettings,
	parseDfu
};

typedef struct CliInput {
	// only one will be populated, see parsedType
	Mode mode;
	ChipSettings settings;

	// metadata
	uint8_t modeIndex;

	// metadata calculated at runtime
	uint16_t jsonLength;

	ModeErrorContext errorContext;
	
	// metadata calculated at runtime
	// 0 for not parsed successfully
	// 1 for mode
	// 2 for settings
	enum ParseResult parsedType;
} CliInput;

// shared preallocated CliInput used for parsing json commands.
extern CliInput cliInput;

#endif /* INC_MODEL_CLI_MODEL_H_ */
