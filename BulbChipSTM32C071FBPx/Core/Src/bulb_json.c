/*
 * BulbJson.c
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */
#include <stdint.h>
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

/**
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
			if ((t = lwjson_find(&lwjson, "mode")) != NULL) {
				BulbMode mode;
				parseModeJson(&lwjson, t, &mode);
				input->mode = mode;
				input->parsedType = 1;
			}

			if ((t = lwjson_find(&lwjson, "index")) != NULL) {
				input->mode.modeIndex = t->u.num_int;
			}
		} else if (strncmp(command, "setSettings", 11) == 0) {
			if ((t = lwjson_find(&lwjson, "settings")) != NULL) {
				// TODO
				input->parsedType = 2;
			}
		} else {
			// unable to parse
			input->parsedType = 0;
		}
	}
	lwjson_free(&lwjson);
}

void parseModeJson(lwjson_t *lwjson, lwjson_token_t *modeJsonObject, BulbMode *mode) {
	const lwjson_token_t *t;

	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "name")) != NULL) {
		char *nameRaw = t->u.str.token_value;
		for (uint8_t i = 0; i < t->u.str.token_value_len; i++) {
			mode->name[i] = nameRaw[i];
		}
		mode->name[t->u.str.token_value_len] = '\0';
	}

	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "totalTicks")) != NULL) {
		mode->totalTicks = t->u.num_int;
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
				}

				if ((tObject = lwjson_find_ex(lwjson, tkn, "tick")) != NULL) {
					tick = tObject->u.num_int;
				}

				ChangeAt change = { tick, output };
				mode->changeAt[changeIndex] = change;
				changeIndex++;
			}
		}
		mode->numChanges = changeIndex;
	}
}
