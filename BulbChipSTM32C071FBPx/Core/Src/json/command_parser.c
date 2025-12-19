/*
 * BulbJson.c
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "json/command_parser.h"
#include "json/mode_parser.h"
#include "lwjson/lwjson.h"
#include "chip_state.h"
#include "storage.h"

CliInput cliInput;

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
	if (lwjson_parse(&lwjson, (const char *)bufJson) == lwjsonOK) {
		const lwjson_token_t *t;
		char command[32];

		if ((t = lwjson_find(&lwjson, "command")) != NULL) {
			const char *nameRaw = t->u.str.token_value;
			for (uint8_t i = 0; i < t->u.str.token_value_len; i++) {
				command[i] = nameRaw[i];
			}
			command[t->u.str.token_value_len] = '\0';
		}

		if (strncmp(command, "writeMode", 9) == 0) {
			bool didParseMode = false;
			bool didParseIndex = false;
			Mode mode;
			ModeErrorContext ctx;
			ctx.path[0] = '\0';
			ctx.error = MODE_PARSER_OK;

			if ((t = lwjson_find(&lwjson, "mode")) != NULL) {
				didParseMode = parseMode(&lwjson, (lwjson_token_t *)t, &mode, &ctx);
				input->mode = mode;
			}

			if ((t = lwjson_find(&lwjson, "index")) != NULL) {
				input->modeIndex = t->u.num_int;
				didParseIndex = true;
			}

			if (didParseMode && didParseIndex) {
				input->parsedType = parseWriteMode;
			}
		} else if (strncmp(command, "readMode", 8) == 0) {
			if ((t = lwjson_find(&lwjson, "index")) != NULL) {
				input->modeIndex = t->u.num_int;
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

void handleJson(uint8_t buf[], uint32_t count) {
	parseJson(buf, count, &cliInput);

	switch (cliInput.parsedType) {
	case parseError: {
		// TODO return errors from mode parser
		char error[] = "{\"error\":\"unable to parse json\"}\n";
		chip_state_write_serial(error);
		break;
	}
	case parseWriteMode: {
		if (strcmp(cliInput.mode.name, "transientTest") == 0) {
			// do not write to flash for transient test
			chip_state_update_mode(cliInput.modeIndex, &cliInput.mode);
		} else {
			writeBulbModeToFlash(cliInput.modeIndex, buf, cliInput.jsonLength);
			chip_state_update_mode(cliInput.modeIndex, &cliInput.mode);
		}
		break;
	}
	case parseReadMode: {
		char flashReadBuffer[1024];
		chip_state_load_mode(cliInput.modeIndex, flashReadBuffer);
		uint16_t len = strlen(flashReadBuffer);
		flashReadBuffer[len] = '\n';
		flashReadBuffer[len + 1] = '\0';
		chip_state_write_serial(flashReadBuffer);
		break;
	}
	case parseWriteSettings: {
		ChipSettings settings = cliInput.settings;
		writeSettingsToFlash(buf, cliInput.jsonLength);
		chip_state_update_settings(&settings);
		break;
	}
	case parseReadSettings: {
		char flashReadBuffer[1024];
		ChipSettings settings;
		chip_state_load_settings(&settings, flashReadBuffer);
		uint16_t len = strlen(flashReadBuffer);
		flashReadBuffer[len] = '\n';
		flashReadBuffer[len + 1] = '\0';
		chip_state_write_serial(flashReadBuffer);
		break;
	}
	case parseDfu: {
		chip_state_enter_dfu();
		break;
	}}

	if (cliInput.parsedType != parseError) {
		chip_state_show_success();
	}
}
