#include "unity.h"
#include <string.h>
#include <stdbool.h>

#include "json/command_parser.h"
#include "json/mode_parser.h"

// Mock Data
CliInput cliInput;
static bool parseModeCalled = false;
static bool parseModeResult = true;

// Mock Functions
bool parseMode(lwjson_t *lwjson, lwjson_token_t *t, Mode *mode, ModeErrorContext *ctx) {
    parseModeCalled = true;
    return parseModeResult;
}

// Include source
#include "../Core/Src/json/command_parser.c"

void setUp(void) {
    memset(&cliInput, 0, sizeof(CliInput));
    parseModeCalled = false;
    parseModeResult = true;
}

void tearDown(void) {}

void test_ParseJson_WriteMode_ParsesIndexAndData(void) {
    char *json = "{\"command\":\"writeMode\",\"index\":5,\"mode\":{}}";
    
    parseJson((uint8_t*)json, strlen(json) + 1, &cliInput);
    
    TEST_ASSERT_EQUAL(parseWriteMode, cliInput.parsedType);
    TEST_ASSERT_EQUAL_UINT8(5, cliInput.modeIndex);
    TEST_ASSERT_TRUE(parseModeCalled);
}

void test_ParseJson_ReadMode_SetsReadAction(void) {
    char *json = "{\"command\":\"readMode\",\"index\":3}";
    
    parseJson((uint8_t*)json, strlen(json) + 1, &cliInput);
    
    TEST_ASSERT_EQUAL(parseReadMode, cliInput.parsedType);
    TEST_ASSERT_EQUAL_UINT8(3, cliInput.modeIndex);
}

void test_ParseJson_WriteSettings_ParsesSettingsValues(void) {
    char *json = "{\"command\":\"writeSettings\",\"modeCount\":10,\"minutesUntilAutoOff\":20,\"minutesUntilLockAfterAutoOff\":30}";
    
    parseJson((uint8_t*)json, strlen(json) + 1, &cliInput);
    
    TEST_ASSERT_EQUAL(parseWriteSettings, cliInput.parsedType);
    TEST_ASSERT_EQUAL_UINT8(10, cliInput.settings.modeCount);
    TEST_ASSERT_EQUAL_UINT8(20, cliInput.settings.minutesUntilAutoOff);
    TEST_ASSERT_EQUAL_UINT8(30, cliInput.settings.minutesUntilLockAfterAutoOff);
}

void test_ParseJson_Dfu_SetsDfuAction(void) {
    char *json = "{\"command\":\"dfu\"}";
    
    parseJson((uint8_t*)json, strlen(json) + 1, &cliInput);
    
    TEST_ASSERT_EQUAL(parseDfu, cliInput.parsedType);
}

void test_ParseJson_InvalidJson_DoesNotCrash(void) {
    char *json = "{invalid json";
    
    parseJson((uint8_t*)json, strlen(json) + 1, &cliInput);
    
    TEST_ASSERT_EQUAL(parseError, cliInput.parsedType);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ParseJson_WriteMode_ParsesIndexAndData);
    RUN_TEST(test_ParseJson_ReadMode_SetsReadAction);
    RUN_TEST(test_ParseJson_WriteSettings_ParsesSettingsValues);
    RUN_TEST(test_ParseJson_Dfu_SetsDfuAction);
    RUN_TEST(test_ParseJson_InvalidJson_DoesNotCrash);
    return UNITY_END();
}
