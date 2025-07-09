/*
 * BulbJson.c
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */
#include <stdint.h>
#include "bulb_json.h"
#include "lwjson/lwjson.h"

static lwjson_token_t tokens[128];
static lwjson_t lwjson;

static uint32_t jsonLength(uint8_t buf[], uint32_t count) {
	for (uint32_t i = 0; i < count; i++) {
		char current = buf[i];
		if (current == '\n' || current == '\0') {
			return i;
		}
	}
	return -1;
}

// this function assumes the json only has a new line at the very end
BulbMode parseJson(uint8_t buf[], uint32_t count) {
	uint32_t indexOfTerminalChar = jsonLength(buf, count);
	uint8_t includeTerimalChar = 1;
	uint8_t bufJson[indexOfTerminalChar + includeTerimalChar];

	for (uint32_t i = 0; i < indexOfTerminalChar + includeTerimalChar; i++) {
		bufJson[i] = buf[i];
	}

	// ensure terminal character is \0 and not \n
	bufJson[indexOfTerminalChar] = '\0';

	BulbMode mode;
	mode.jsonLength = indexOfTerminalChar;

	lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
	if (lwjson_parse(&lwjson, bufJson) == lwjsonOK) {
		const lwjson_token_t *t;

		if ((t = lwjson_find(&lwjson, "name")) != NULL) {
			char *nameRaw = t->u.str.token_value;
			for (uint8_t i = 0; i < t->u.str.token_value_len; i++) {
				mode.name[i] = nameRaw[i];
			}
			mode.name[t->u.str.token_value_len] = '\0';
		}

		if ((t = lwjson_find(&lwjson, "modeIndex")) != NULL) {
			mode.modeIndex = t->u.num_int;
		}

		if ((t = lwjson_find(&lwjson, "totalTicks")) != NULL) {
			mode.totalTicks = t->u.num_int;
		}

		if ((t = lwjson_find(&lwjson, "changeAt")) != NULL) {
			uint8_t changeIndex = 0;
			for (const lwjson_token_t *tkn = lwjson_get_first_child(t);
					tkn != NULL; tkn = tkn->next) {
				if (tkn->type == LWJSON_TYPE_OBJECT) {
					const lwjson_token_t *tObject;
					enum Output output;
					uint16_t tick;

					if ((tObject = lwjson_find_ex(&lwjson, tkn, "output"))
							!= NULL) {
						if (strncmp(tObject->u.str.token_value, "high", 4)
								== 0) {
							output = high;
						} else {
							output = low;
						}
					}

					if ((tObject = lwjson_find_ex(&lwjson, tkn, "tick")) != NULL) {
						tick = tObject->u.num_int;
					}

					ChangeAt change = { tick, output };
					mode.changeAt[changeIndex] = change;
					changeIndex++;
				}
			}
			mode.numChanges = changeIndex;
		}
		lwjson_free(&lwjson);
	}

	return mode;
}
