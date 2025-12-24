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

static uint32_t jsonLength(const uint8_t buf[], uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        uint8_t current = buf[i];
        if (current == '\n' || current == '\0') {
            return i;
        }
    }
    return -1;
}

static bool parseSettingsJson(lwjson_t *lwjson, ChipSettings *settings) {
    const lwjson_token_t *token;

    uint8_t parsedProperties = 0;

    token = lwjson_find(lwjson, "modeCount");
    if (token != NULL) {
        settings->modeCount = token->u.num_int;
        parsedProperties++;
    }

    token = lwjson_find(lwjson, "minutesUntilAutoOff");
    if (token != NULL) {
        settings->minutesUntilAutoOff = token->u.num_int;
        parsedProperties++;
    }

    token = lwjson_find(lwjson, "minutesUntilLockAfterAutoOff");
    if (token != NULL) {
        settings->minutesUntilLockAfterAutoOff = token->u.num_int;
        parsedProperties++;
    }

    return parsedProperties == 3;
}

static void handleWriteMode(lwjson_t *lwjson, CliInput *input) {
    const lwjson_token_t *token;
    bool didParseMode = false;
    bool didParseIndex = false;
    Mode mode;

    input->errorContext.path[0] = '\0';
    input->errorContext.error = MODE_PARSER_OK;

    token = lwjson_find(lwjson, "mode");
    if (token != NULL) {
        didParseMode = parseMode(lwjson, (lwjson_token_t *)token, &mode, &input->errorContext);
        input->mode = mode;
    }

    token = lwjson_find(lwjson, "index");
    if (token != NULL) {
        input->modeIndex = token->u.num_int;
        didParseIndex = true;
    }

    if (didParseMode && didParseIndex) {
        input->parsedType = parseWriteMode;
    }
}

static void handleReadMode(lwjson_t *lwjson, CliInput *input) {
    const lwjson_token_t *token = lwjson_find(lwjson, "index");
    if (token != NULL) {
        input->modeIndex = token->u.num_int;
        input->parsedType = parseReadMode;
    }
}

static void handleWriteSettings(lwjson_t *lwjson, CliInput *input) {
    ChipSettings settings;
    if (parseSettingsJson(lwjson, &settings)) {
        input->settings = settings;
        input->parsedType = parseWriteSettings;
    }
}

static void processCommand(lwjson_t *lwjson, CliInput *input) {
    const lwjson_token_t *token;
    char command[32] = {0};

    token = lwjson_find(lwjson, "command");
    if (token == NULL) {
        return;
    }

    const char *nameRaw = token->u.str.token_value;
    size_t len = token->u.str.token_value_len;
    if (len >= sizeof(command)) {
        len = sizeof(command) - 1;
    }

    memcpy(command, nameRaw, len);
    command[len] = '\0';

    if (strncmp(command, "writeMode", 9) == 0) {
        handleWriteMode(lwjson, input);
    } else if (strncmp(command, "readMode", 8) == 0) {
        handleReadMode(lwjson, input);
    } else if (strncmp(command, "writeSettings", 13) == 0) {
        handleWriteSettings(lwjson, input);
    } else if (strncmp(command, "readSettings", 12) == 0) {
        input->parsedType = parseReadSettings;
    } else if (strncmp(command, "dfu", 3) == 0) {
        input->parsedType = parseDfu;
    }
}

void parseJson(const uint8_t buf[], uint32_t count, CliInput *input) {
    static lwjson_token_t tokens[128];
    static lwjson_t lwjson;

    if (input == NULL) {
        return;
    }

    if (buf == NULL || count == 0) {
        input->parsedType = parseError;
        return;
    }

    uint32_t indexOfTerminalChar = jsonLength(buf, count);
    if (indexOfTerminalChar == -1) {
        input->parsedType = parseError;
        return;
    }

    uint8_t includeTerimalChar = 1;
    uint8_t bufJson[indexOfTerminalChar + includeTerimalChar];

    for (uint32_t i = 0; i < indexOfTerminalChar + includeTerimalChar; i++) {
        bufJson[i] = buf[i];
    }

    // ensure terminal character is \0 and not \n
    bufJson[indexOfTerminalChar] = '\0';
    input->jsonLength = indexOfTerminalChar;

    input->parsedType = parseError;  // provide error default, override when successful
    input->errorContext.error = MODE_PARSER_OK;
    input->errorContext.path[0] = '\0';

    lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
    if (lwjson_parse(&lwjson, (const char *)bufJson) == lwjsonOK) {
        processCommand(&lwjson, input);
    }
    lwjson_free(&lwjson);
}
