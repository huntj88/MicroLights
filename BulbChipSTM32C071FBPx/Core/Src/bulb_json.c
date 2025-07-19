/*
 * BulbJson.c
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */
#include <stdint.h>
#include <stdbool.h>
#include "bulb_json.h"
#include "lwjson/lwjson.h"

static uint32_t jsonLength(uint8_t buf[], uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		char current = buf[i];
		if (current == '\n' || current == '\0') {
			return i;
		}
	}
	return -1;
}

static bool parseSettingsJson(lwjson_t *lwjson, ChipSettings *settings) {
	const lwjson_token_t *t;

	uint8_t parsedProperties = 0;

	if ((t = lwjson_find(lwjson, "modeCount")) != NULL) {
		settings->modeCount = t->u.num_int;
		parsedProperties++;
	}

	if ((t = lwjson_find(lwjson, "minutesUntilAutoOff")) != NULL) {
		settings->minutesUntilAutoOff = t->u.num_int;
		parsedProperties++;
	}

	if ((t = lwjson_find(lwjson, "minutesUntilLockAfterAutoOff")) != NULL) {
		settings->minutesUntilLockAfterAutoOff = t->u.num_int;
		parsedProperties++;
	}

	return parsedProperties == 3;
}

static bool parseModeJson(lwjson_t *lwjson, lwjson_token_t *modeJsonObject, BulbMode *mode) {
	const lwjson_token_t *t;

	bool didParseName = false;
	bool didParseTotalTicks = false;
	bool didParseOutput = false;
	bool didParseTick = false;

	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "name")) != NULL) {
		char *nameRaw = t->u.str.token_value;
		for (uint8_t i = 0; i < t->u.str.token_value_len; i++) {
			mode->name[i] = nameRaw[i];
		}
		mode->name[t->u.str.token_value_len] = '\0';
		didParseName = true;
	}

	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "totalTicks")) != NULL) {
		mode->totalTicks = t->u.num_int;
		didParseTotalTicks = true;
	}

	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "changeAt")) != NULL) {
		uint8_t changeIndex = 0;
		for (const lwjson_token_t *tkn = lwjson_get_first_child(t);
				tkn != NULL; tkn = tkn->next) {
			if (tkn->type == LWJSON_TYPE_OBJECT) {
				const lwjson_token_t *tObject;
				enum Output output;
				uint16_t tick;

				if ((tObject = lwjson_find_ex(lwjson, tkn, "output"))
						!= NULL) {
					if (strncmp(tObject->u.str.token_value, "high", 4)
							== 0) {
						output = high;
					} else {
						output = low;
					}
					didParseOutput = true;
				}

				if ((tObject = lwjson_find_ex(lwjson, tkn, "tick")) != NULL) {
					tick = tObject->u.num_int;
					didParseTick = true;
				}

				ChangeAt change = { tick, output };
				mode->changeAt[changeIndex] = change;
				changeIndex++;
			}
		}
		mode->numChanges = changeIndex;
	}

	bool hasMinRequiredProperties = didParseName && didParseTotalTicks && didParseOutput && didParseTick;

	return hasMinRequiredProperties && mode->totalTicks > 0;
}

/**
 * Example commands
 *
{
  "command": "setMode",
  "index": 1,
  "mode": {
    "name": "blah0",
    "totalTicks": 3,
    "changeAt": [
      {
        "tick": 0,
        "output": "low"
      },
      {
        "tick": 1,
        "output": "high"
      }
    ]
  }
}

{
  "command": "writeSettings",
  "modeCount": 3,
  "minutesUntilAutoOff": 90,
  "minutesUntilLockAfterAutoOff": 10
}

{"command":"readSettings"}

{"command":"dfu"}
*/
void parseJson(uint8_t buf[], uint32_t count, CliInput *input) {
	static lwjson_token_t tokens[128];
	static lwjson_t lwjson;

	uint32_t indexOfTerminalChar = jsonLength(buf, count);
	uint8_t includeTerimalChar = 1;
	uint8_t bufJson[indexOfTerminalChar + includeTerimalChar];

	for (uint32_t i = 0; i < indexOfTerminalChar + includeTerimalChar; i++) {
		bufJson[i] = buf[i];
	}

	// ensure terminal character is \0 and not \n
	bufJson[indexOfTerminalChar] = '\0';
	input->jsonLength = indexOfTerminalChar;

	input->parsedType = parseError; // provide error default, override when successful

	lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
	if (lwjson_parse(&lwjson, bufJson) == lwjsonOK) {
		const lwjson_token_t *t;
		char command[32];

		if ((t = lwjson_find(&lwjson, "command")) != NULL) {
			char *nameRaw = t->u.str.token_value;
			for (uint8_t i = 0; i < t->u.str.token_value_len; i++) {
				command[i] = nameRaw[i];
			}
			command[t->u.str.token_value_len] = '\0';
		}

		if (strncmp(command, "setMode", 7) == 0) {
			bool didParseMode = false;
			bool didParseIndex = false;
			BulbMode mode;

			if ((t = lwjson_find(&lwjson, "mode")) != NULL) {
				didParseMode = parseModeJson(&lwjson, t, &mode);
				input->mode = mode;
			}

			if ((t = lwjson_find(&lwjson, "index")) != NULL) {
				input->mode.modeIndex = t->u.num_int;
				didParseIndex = true;
			}

			if (didParseMode && didParseIndex) {
				input->parsedType = parseMode;
			}
		} else if (strncmp(command, "writeSettings", 13) == 0) {
			ChipSettings settings;
			if (parseSettingsJson(&lwjson, &settings)) {
				input->settings = settings;
				input->parsedType = parseWriteSettings;
			}
		} else if (strncmp(command, "dfu", 3) == 0) {
			input->parsedType = parseDfu;
		} else if (strncmp(command, "readSettings", 12) == 0) {
			input->parsedType = parseReadSettings;
		}
	}
	lwjson_free(&lwjson);
}
