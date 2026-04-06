/*
 * BulbJson.c
 *
 *  Created on: Jun 28, 2025
 *      Author: jameshunt
 */
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lwjson/lwjson.h"
#include "microlight/json/command_parser.h"
#include "microlight/json/json_buf.h"
#include "microlight/json/mode_parser.h"

static int32_t jsonLength(const char *buffer, size_t length) {
    for (size_t i = 0; i < length; i++) {
        char current = buffer[i];
        if (current == '\n' || current == '\0') {
            return (int32_t)i;
        }
    }
    return -1;
}

static bool setParserError(ParserErrorContext *ctx, ParserError error, const char *path) {
    ctx->error = error;
    snprintf(ctx->path, sizeof(ctx->path), "%s", path);
    return false;
}

static bool parseUint8Setting(
    lwjson_t *lwjson,
    const char *path,
    uint8_t maxValue,
    uint8_t *destination,
    ParserErrorContext *ctx) {
    const lwjson_token_t *token = lwjson_find(lwjson, path);
    if (token == NULL) {
        return true;
    }

    if (token->type != LWJSON_TYPE_NUM_INT) {
        return setParserError(ctx, PARSER_ERR_INVALID_VARIANT, path);
    }

    if (token->u.num_int < 0) {
        return setParserError(ctx, PARSER_ERR_VALUE_TOO_SMALL, path);
    }
    if (token->u.num_int > maxValue) {
        return setParserError(ctx, PARSER_ERR_VALUE_TOO_LARGE, path);
    }

    *destination = (uint8_t)token->u.num_int;
    return true;
}

static bool parseBoolSetting(
    lwjson_t *lwjson, const char *path, bool *destination, ParserErrorContext *ctx) {
    const lwjson_token_t *token = lwjson_find(lwjson, path);
    if (token == NULL) {
        return true;
    }

    if (token->type == LWJSON_TYPE_TRUE) {
        *destination = true;
        return true;
    }
    if (token->type == LWJSON_TYPE_FALSE) {
        *destination = false;
        return true;
    }

    return setParserError(ctx, PARSER_ERR_INVALID_VARIANT, path);
}

static uint8_t maxUint8SettingValue(const char *path) {
    if (strcmp(path, "modeCount") == 0) {
        return 7U;
    }
    if (strcmp(path, "shutdownPolicy") == 0) {
        return (uint8_t)autoOffAndAutoLock;
    }
    return UINT8_MAX;
}

#define PARSE_SETTING_uint8_t(name, def)                                                        \
    if (!parseUint8Setting(lwjson, #name, maxUint8SettingValue(#name), &settings->name, ctx)) { \
        return false;                                                                           \
    }

#define PARSE_SETTING_bool(name, def)                             \
    if (!parseBoolSetting(lwjson, #name, &settings->name, ctx)) { \
        return false;                                             \
    }

#define PARSE_SETTING(type, name, def) PARSE_SETTING_##type(name, def)

static bool parseSettingsJson(lwjson_t *lwjson, ChipSettings *settings, ParserErrorContext *ctx) {
    CHIP_SETTINGS_MAP(PARSE_SETTING);

    return true;
}

#undef PARSE_SETTING
#undef PARSE_SETTING_bool
#undef PARSE_SETTING_uint8_t

static void handleWriteMode(lwjson_t *lwjson, CliInput *input) {
    const lwjson_token_t *token;
    bool didParseMode = false;
    bool didParseIndex = false;
    Mode mode;

    input->errorContext.path[0] = '\0';
    input->errorContext.error = PARSER_OK;

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
    chipSettingsInitDefaults(&settings);
    if (parseSettingsJson(lwjson, &settings, &input->errorContext)) {
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

void parseJson(const char *buffer, size_t length, CliInput *input) {
    static lwjson_token_t tokens[128];
    static lwjson_t lwjson;

    if (input == NULL) {
        return;
    }

    if (buffer == NULL || length == 0) {
        input->parsedType = parseError;
        return;
    }

    int32_t indexOfTerminalChar = jsonLength(buffer, length);
    if (indexOfTerminalChar == -1 || indexOfTerminalChar >= sharedJsonIOBufferLength - 1U) {
        input->parsedType = parseError;
        return;
    }

    if (buffer != sharedJsonIOBuffer) {
        memcpy(sharedJsonIOBuffer, buffer, indexOfTerminalChar);
    }

    // ensure terminal character is \0 and not \n
    sharedJsonIOBuffer[indexOfTerminalChar] = '\0';

    input->parsedType = parseError;  // provide error default, override when successful
    input->errorContext.error = PARSER_OK;
    input->errorContext.path[0] = '\0';

    lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
    if (lwjson_parse(&lwjson, sharedJsonIOBuffer) == lwjsonOK) {
        processCommand(&lwjson, input);
    }
    lwjson_free(&lwjson);
}
