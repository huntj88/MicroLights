#include <stdbool.h>
#include <string.h>
#include "unity.h"

#include "microlight/json/command_parser.h"
#include "microlight/json/mode_parser.h"
#include "microlight/model/cli_model.h"

static bool parseModeCalled = false;
static bool parseModeResult = true;

// Mock Functions
bool parseMode(lwjson_t *lwjson, lwjson_token_t *t, Mode *mode, ParserErrorContext *ctx) {
    parseModeCalled = true;
    return parseModeResult;
}

// Include source
#include "../Core/Src/microlight/json/command_parser.c"

void setUp(void) {
    parseModeCalled = false;
    parseModeResult = true;
}

void tearDown(void) {
}

void test_ParseJson_WriteMode_ParsesIndexAndData(void) {
    char *json = "{\"command\":\"writeMode\",\"index\":5,\"mode\":{}}";

    parseJson((uint8_t *)json, strlen(json) + 1, &cliInput);

    TEST_ASSERT_EQUAL(parseWriteMode, cliInput.parsedType);
    TEST_ASSERT_EQUAL_UINT8(5, cliInput.modeIndex);
    TEST_ASSERT_TRUE(parseModeCalled);
}

void test_ParseJson_ReadMode_SetsReadAction(void) {
    char *json = "{\"command\":\"readMode\",\"index\":3}";

    parseJson((uint8_t *)json, strlen(json) + 1, &cliInput);

    TEST_ASSERT_EQUAL(parseReadMode, cliInput.parsedType);
    TEST_ASSERT_EQUAL_UINT8(3, cliInput.modeIndex);
}

void test_ParseJson_WriteSettings_ParsesBooleanValues(void) {
    char *json = "{\"command\":\"writeSettings\",\"enableChargerSerial\":true}";

    parseJson((uint8_t *)json, strlen(json) + 1, &cliInput);

    TEST_ASSERT_EQUAL(parseWriteSettings, cliInput.parsedType);
    TEST_ASSERT_TRUE(cliInput.settings.enableChargerSerial);

    char *json2 = "{\"command\":\"writeSettings\",\"enableChargerSerial\":false}";

    parseJson((uint8_t *)json2, strlen(json2) + 1, &cliInput);

    TEST_ASSERT_EQUAL(parseWriteSettings, cliInput.parsedType);
    TEST_ASSERT_FALSE(cliInput.settings.enableChargerSerial);
}

void test_ParseJson_WriteSettings_RejectsIntegerForBoolean(void) {
    char *json = "{\"command\":\"writeSettings\",\"enableChargerSerial\":1}";

    parseJson((uint8_t *)json, strlen(json) + 1, &cliInput);

    // Should fail to parse as writeSettings because of the error
    // The parser sets parsedType to parseError initially.
    // If parseSettingsJson returns false, handleWriteSettings won't set parsedType to
    // parseWriteSettings.
    TEST_ASSERT_EQUAL(parseError, cliInput.parsedType);
    TEST_ASSERT_EQUAL(PARSER_ERR_INVALID_VARIANT, cliInput.errorContext.error);
}

void test_ParseJson_WriteSettings_ParsesSettingsValues(void) {
    char *json =
        "{\"command\":\"writeSettings\",\"modeCount\":7,\"minutesUntilAutoOff\":20,"
        "\"minutesUntilLockAfterAutoOff\":30,\"equationEvalIntervalMs\":50}";

    parseJson((uint8_t *)json, strlen(json) + 1, &cliInput);

    TEST_ASSERT_EQUAL(parseWriteSettings, cliInput.parsedType);
    TEST_ASSERT_EQUAL_UINT8(7, cliInput.settings.modeCount);
    TEST_ASSERT_EQUAL_UINT8(20, cliInput.settings.minutesUntilAutoOff);
    TEST_ASSERT_EQUAL_UINT8(30, cliInput.settings.minutesUntilLockAfterAutoOff);
    TEST_ASSERT_EQUAL_UINT8(50, cliInput.settings.equationEvalIntervalMs);
}

void test_ParseJson_WriteSettings_RejectsInvalidValues(void) {
    // Invalid modeCount (>7)
    char *json1 = "{\"command\":\"writeSettings\",\"modeCount\":8}";
    parseJson((uint8_t *)json1, strlen(json1) + 1, &cliInput);
    TEST_ASSERT_EQUAL(parseError, cliInput.parsedType);
    TEST_ASSERT_EQUAL(PARSER_ERR_VALUE_TOO_LARGE, cliInput.errorContext.error);
    TEST_ASSERT_EQUAL_STRING("modeCount", cliInput.errorContext.path);

    // Invalid minutesUntilAutoOff (>255)
    char *json2 = "{\"command\":\"writeSettings\",\"minutesUntilAutoOff\":256}";
    parseJson((uint8_t *)json2, strlen(json2) + 1, &cliInput);
    TEST_ASSERT_EQUAL(parseError, cliInput.parsedType);
    TEST_ASSERT_EQUAL(PARSER_ERR_VALUE_TOO_LARGE, cliInput.errorContext.error);
    TEST_ASSERT_EQUAL_STRING("minutesUntilAutoOff", cliInput.errorContext.path);

    // Invalid minutesUntilLockAfterAutoOff (>255)
    char *json3 = "{\"command\":\"writeSettings\",\"minutesUntilLockAfterAutoOff\":256}";
    parseJson((uint8_t *)json3, strlen(json3) + 1, &cliInput);
    TEST_ASSERT_EQUAL(parseError, cliInput.parsedType);
    TEST_ASSERT_EQUAL(PARSER_ERR_VALUE_TOO_LARGE, cliInput.errorContext.error);
    TEST_ASSERT_EQUAL_STRING("minutesUntilLockAfterAutoOff", cliInput.errorContext.path);

    // Invalid equationEvalIntervalMs (>255)
    char *json4 = "{\"command\":\"writeSettings\",\"equationEvalIntervalMs\":256}";
    parseJson((uint8_t *)json4, strlen(json4) + 1, &cliInput);
    TEST_ASSERT_EQUAL(parseError, cliInput.parsedType);
    TEST_ASSERT_EQUAL(PARSER_ERR_VALUE_TOO_LARGE, cliInput.errorContext.error);
    TEST_ASSERT_EQUAL_STRING("equationEvalIntervalMs", cliInput.errorContext.path);
}

void test_ParseJson_Dfu_SetsDfuAction(void) {
    char *json = "{\"command\":\"dfu\"}";

    parseJson((uint8_t *)json, strlen(json) + 1, &cliInput);

    TEST_ASSERT_EQUAL(parseDfu, cliInput.parsedType);
}

void test_ParseJson_InvalidJson_DoesNotCrash(void) {
    char *json = "{invalid json";

    parseJson((uint8_t *)json, strlen(json) + 1, &cliInput);

    TEST_ASSERT_EQUAL(parseError, cliInput.parsedType);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ParseJson_Dfu_SetsDfuAction);
    RUN_TEST(test_ParseJson_InvalidJson_DoesNotCrash);
    RUN_TEST(test_ParseJson_ReadMode_SetsReadAction);
    RUN_TEST(test_ParseJson_WriteMode_ParsesIndexAndData);
    RUN_TEST(test_ParseJson_WriteSettings_ParsesBooleanValues);
    RUN_TEST(test_ParseJson_WriteSettings_ParsesSettingsValues);
    RUN_TEST(test_ParseJson_WriteSettings_RejectsIntegerForBoolean);
    RUN_TEST(test_ParseJson_WriteSettings_RejectsInvalidValues);
    return UNITY_END();
}
