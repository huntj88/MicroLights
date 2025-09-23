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

static bool parseWaveformJson(lwjson_t *lwjson, lwjson_token_t *waveformJsonObject, Waveform *waveform) {
	const lwjson_token_t *t;
	bool didParseName = false;
	bool didParseTotalTicks = false;
	bool didParseOutput = false;
	bool didParseTick = false;

	if ((t = lwjson_find_ex(lwjson, waveformJsonObject, "name")) != NULL) {
		char *nameRaw = t->u.str.token_value;
		for (uint8_t i = 0; i < t->u.str.token_value_len; i++) {
			waveform->name[i] = nameRaw[i];
		}
		waveform->name[t->u.str.token_value_len] = '\0';
		didParseName = true;
	}

	if ((t = lwjson_find_ex(lwjson, waveformJsonObject, "totalTicks")) != NULL) {
		waveform->totalTicks = t->u.num_int;
		didParseTotalTicks = true;
	}

	if ((t = lwjson_find_ex(lwjson, waveformJsonObject, "changeAt")) != NULL) {
		uint8_t changeIndex = 0;
		for (const lwjson_token_t *tkn = lwjson_get_first_child(t);
			tkn != NULL; tkn = tkn->next) {
			if (tkn->type == LWJSON_TYPE_OBJECT) {
				const lwjson_token_t *tObject;
				enum Output output;
				uint16_t tick;

				if ((tObject = lwjson_find_ex(lwjson, tkn, "output"))
					!= NULL) {
					if (strncmp(tObject->u.str.token_value, "high", 4) == 0) {
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
				waveform->changeAt[changeIndex] = change;
				changeIndex++;
			}
		}
		waveform->numChanges = changeIndex;
	}

	bool hasMinRequiredProperties = didParseName && didParseTotalTicks && didParseOutput && didParseTick;

	return hasMinRequiredProperties && waveform->totalTicks > 0;
}

static bool parseModeJson(lwjson_t *lwjson, lwjson_token_t *modeJsonObject, BulbMode *mode) {
	const lwjson_token_t *t;
	bool didParseName = false;
	bool didParseColor = false;
	bool didParseWaveform = false;
	bool didParseAccel = false;

	// name
	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "name")) != NULL) {
		char *nameRaw = t->u.str.token_value;
		for (uint8_t i = 0; i < t->u.str.token_value_len; i++) {
			mode->name[i] = nameRaw[i];
		}
		mode->name[t->u.str.token_value_len] = '\0';
		didParseName = true;
	}

	// color
	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "color")) != NULL) {
		char *colorRaw = t->u.str.token_value;
		uint8_t len = t->u.str.token_value_len < (sizeof(mode->color) - 1) ? t->u.str.token_value_len : (sizeof(mode->color) - 1);
		for (uint8_t i = 0; i < len; i++) {
			mode->color[i] = colorRaw[i];
		}
		mode->color[len] = '\0';
		didParseColor = true;
	}

	// waveform
	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "waveform")) != NULL) {
		didParseWaveform = parseWaveformJson(lwjson, (lwjson_token_t *)t, &mode->waveform);
	}

	// accel -> triggers
	mode->triggerCount = 0;
	if ((t = lwjson_find_ex(lwjson, modeJsonObject, "accel")) != NULL) {
		const lwjson_token_t *tTriggers = lwjson_find_ex(lwjson, (lwjson_token_t *)t, "triggers");
		if (tTriggers != NULL) {
			uint8_t triggerIndex = 0;
			for (const lwjson_token_t *tkn = lwjson_get_first_child(tTriggers);
				tkn != NULL; tkn = tkn->next) {
				if (tkn->type == LWJSON_TYPE_OBJECT) {
					const lwjson_token_t *tObject;
					AccelTrigger trigger;
					// threshold
					if ((tObject = lwjson_find_ex(lwjson, tkn, "threshold")) != NULL) {
						trigger.threshold = tObject->u.num_int;
					} else {
						continue; // skip malformed trigger
					}
					// color
					if ((tObject = lwjson_find_ex(lwjson, tkn, "color")) != NULL) {
						char *colorRaw = tObject->u.str.token_value;
						uint8_t len = tObject->u.str.token_value_len < (sizeof(trigger.color) - 1) ? tObject->u.str.token_value_len : (sizeof(trigger.color) - 1);
						for (uint8_t i = 0; i < len; i++) {
							trigger.color[i] = colorRaw[i];
						}
						trigger.color[len] = '\0';
					} else {
						trigger.color[0] = '\0';
					}
					// waveform
					if ((tObject = lwjson_find_ex(lwjson, tkn, "waveform")) != NULL) {
						if (!parseWaveformJson(lwjson, (lwjson_token_t *)tObject, &trigger.waveform)) {
							continue; // skip malformed trigger
						}
					} else {
						continue; // skip malformed trigger
					}

					mode->triggers[triggerIndex] = trigger;
					triggerIndex++;
				}
			}
			mode->triggerCount = triggerIndex;
			didParseAccel = triggerIndex > 0;
		}
	}

	// require name and waveform at minimum
	bool hasMin = didParseName && didParseWaveform && didParseColor;
	return hasMin;
}

/**
 * Example commands
 *
{
  "command": "writeMode",
  "index": 0,
  "mode": {
    "name": "blah0",
	"color": "#3584e4",
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
  "command": "writeMode",
  "index": 1,
  "mode": {
    "name": "New Pattern",
    "color": "#3584e4",
    "waveform": {
      "name": "Pulse",
      "totalTicks": 20,
      "changeAt": [
        {
          "tick": 0,
          "output": "high"
        },
        {
          "tick": 10,
          "output": "low"
        }
      ]
    },
    "accel": {
      "triggers": [
        {
          "threshold": 2,
          "color": "#3584e4",
          "waveform": {
            "name": "Pulse",
            "totalTicks": 20,
            "changeAt": [
              {
                "tick": 0,
                "output": "high"
              },
              {
                "tick": 10,
                "output": "low"
              }
            ]
          }
        },
        {
          "threshold": 4,
          "color": "#3584e4",
          "waveform": {
            "name": "Double Blink",
            "totalTicks": 20,
            "changeAt": [
              {
                "tick": 0,
                "output": "high"
              },
              {
                "tick": 3,
                "output": "low"
              },
              {
                "tick": 6,
                "output": "high"
              },
              {
                "tick": 9,
                "output": "low"
              }
            ]
          }
        }
      ]
    }
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

		if (strncmp(command, "writeMode", 9) == 0) {
			bool didParseMode = false;
			bool didParseIndex = false;
			BulbMode mode;

			if ((t = lwjson_find(&lwjson, "mode")) != NULL) {
				didParseMode = parseModeJson(&lwjson, (lwjson_token_t *)t, &mode);
				input->mode = mode;
			}

			if ((t = lwjson_find(&lwjson, "index")) != NULL) {
				// TODO: validate this works correctly
				input->mode.modeIndex = t->u.num_int;
				didParseIndex = true;
			}

			if (didParseMode && didParseIndex) {
				input->parsedType = parseWriteMode;
			}
		} else if (strncmp(command, "readMode", 8) == 0) {
			BulbMode mode;
			if ((t = lwjson_find(&lwjson, "index")) != NULL) {
				input->mode.modeIndex = t->u.num_int;
				input->parsedType = parseReadMode;
			}
		} else if (strncmp(command, "writeSettings", 13) == 0) {
			ChipSettings settings;
			if (parseSettingsJson(&lwjson, &settings)) {
				input->settings = settings;
				input->parsedType = parseWriteSettings;
			}
		} else if (strncmp(command, "readSettings", 12) == 0) {
			input->parsedType = parseReadSettings;
		} else if (strncmp(command, "dfu", 3) == 0) {
			input->parsedType = parseDfu;
		}
	}
	lwjson_free(&lwjson);
}
