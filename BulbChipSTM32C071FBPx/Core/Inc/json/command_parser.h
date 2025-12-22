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
#include "model/cli_model.h"

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

void parseJson(uint8_t buf[], uint32_t count, CliInput *input);

#endif /* INC_COMMAND_PARSER_H_ */
