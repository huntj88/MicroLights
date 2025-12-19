/*
 * cli_model.h
 *
 *  Created on: Dec 19, 2025
 *      Author: GitHub Copilot
 */

#ifndef INC_MODEL_CLI_MODEL_H_
#define INC_MODEL_CLI_MODEL_H_

#include <stdint.h>
#include "model/mode.h"
#include "settings_manager.h"

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
	
	// metadata calculated at runtime
	// 0 for not parsed successfully
	// 1 for mode
	// 2 for settings
	enum ParseResult parsedType;
} CliInput;

#endif /* INC_MODEL_CLI_MODEL_H_ */
