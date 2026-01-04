#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "unity.h"

#include "chip_state.h"
#include "device/bq25180.h"
#include "device/button.h"
#include "device/mc3479.h"
#include "device/rgb_led.h"
#include "lwjson/lwjson.h"
#include "mode_manager.h"
#include "model/cli_model.h"
#include "settings_manager.h"
#include "storage.h"

// Mock Data
static ModeManager mockModeManager;
static Button mockButton;
static BQ25180 mockCharger;
static MC3479 mockAccel;
static RGBLed mockCaseLed;
static char lastSerialOutput[100];
char jsonBuf[PAGE_SECTOR];

// Mocks for settings_manager.c
CliInput cliInput;
bool parseJsonCalled = false;
void parseJson(const char buf[], uint32_t count, CliInput *input) {
    parseJsonCalled = true;
    // Do nothing, simulating empty/invalid flash or just checking defaults before parse
}

void mock_readSettingsFromFlash(char buffer[], uint32_t length) {
    // Simulate empty flash
    memset(buffer, 0, length);
}

// Mocks for chip_state.c
uint32_t mock_convertTicksToMs(uint32_t ticks) {
    return ticks * 10;
}

void mock_writeUsbSerial(const char *buf, uint32_t count) {
    strncpy(lastSerialOutput, buf, count);
    lastSerialOutput[count] = '\0';
}

enum ChargeState getChargingState(BQ25180 *dev) {
    return notConnected;
}
void loadMode(ModeManager *manager, uint8_t index) {
}
enum ButtonResult buttonInputTask(Button *button, uint32_t ms) {
    return ignore;
}
void rgbShowSuccess(RGBLed *led) {
}
void lock(BQ25180 *dev) {
}
void rgbTask(RGBLed *led, uint32_t ms) {
}
void mc3479Task(MC3479 *dev, uint32_t ms) {
}
void chargerTask(
    BQ25180 *dev, uint32_t ms, bool unplugLockEnabled, bool chargeLedEnabled, bool serialEnabled) {
}
void modeTask(
    ModeManager *manager, uint32_t ms, bool canUpdateCaseLed, uint8_t equationEvalIntervalMs) {
}
bool isFakeOff(ModeManager *manager) {
    return false;
}
bool isEvaluatingButtonPress(Button *button) {
    return false;
}
void fakeOffMode(ModeManager *manager, bool enableLedTimers) {
}

// Include source files under test
#include "../Core/Src/chip_state.c"
#include "../Core/Src/model/mode_state.c"
#include "../Core/Src/settings_manager.c"

void setUp(void) {
    memset(&mockModeManager, 0, sizeof(ModeManager));
    memset(&mockButton, 0, sizeof(Button));
    memset(&mockCharger, 0, sizeof(BQ25180));
    memset(&mockAccel, 0, sizeof(MC3479));
    memset(&mockCaseLed, 0, sizeof(RGBLed));
    state = (ChipState){0};
}

void tearDown(void) {
}

void test_UpdateSettings_UpdatesChipStateSettings(void) {
    // 1. Setup SettingsManager
    SettingsManager settingsManager;
    memset(&settingsManager, 0, sizeof(SettingsManager));

    // Initialize currentSettings manually
    settingsManager.currentSettings.modeCount = 5;
    settingsManager.currentSettings.minutesUntilAutoOff = 10;

    // 2. Configure ChipState with pointer to settingsManager.currentSettings
    configureChipState(
        &mockModeManager,
        &settingsManager.currentSettings,  // Pass the pointer!
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    // 3. Verify initial state
    TEST_ASSERT_EQUAL_UINT8(5, state.settings->modeCount);

    // 4. Create new settings
    ChipSettings newSettings;
    memset(&newSettings, 0, sizeof(ChipSettings));
    newSettings.modeCount = 10;
    newSettings.minutesUntilAutoOff = 20;
    newSettings.minutesUntilLockAfterAutoOff = 5;
    newSettings.equationEvalIntervalMs = 50;

    // 5. Call updateSettings
    updateSettings(&settingsManager, &newSettings);

    // 6. Verify ChipState sees the change
    TEST_ASSERT_EQUAL_UINT8(10, state.settings->modeCount);
    TEST_ASSERT_EQUAL_UINT16(20, state.settings->minutesUntilAutoOff);
    TEST_ASSERT_EQUAL_UINT8(50, state.settings->equationEvalIntervalMs);
}

void test_SettingsManagerInit_SetsDefaults(void) {
    SettingsManager settingsManager;
    memset(&settingsManager, 0, sizeof(SettingsManager));

    // Reset mock state
    parseJsonCalled = false;
    cliInput.parsedType = parseError;  // Ensure parseJson doesn't trigger updateSettings

    settingsManagerInit(&settingsManager, mock_readSettingsFromFlash);

    TEST_ASSERT_EQUAL_UINT8(0, settingsManager.currentSettings.modeCount);
    TEST_ASSERT_EQUAL_UINT8(90, settingsManager.currentSettings.minutesUntilAutoOff);
    TEST_ASSERT_EQUAL_UINT8(10, settingsManager.currentSettings.minutesUntilLockAfterAutoOff);
    TEST_ASSERT_EQUAL_UINT8(20, settingsManager.currentSettings.equationEvalIntervalMs);
}

void mock_readSettingsFromFlash_Matching(char buffer[], uint32_t length) {
    snprintf(buffer, length, "{\"modeCount\":0,\"equationEvalIntervalMs\":20}");
}

void test_SettingsManagerInit_DoesNotWriteFlash_WhenFlashMatches(void) {
    SettingsManager settingsManager;
    memset(&settingsManager, 0, sizeof(SettingsManager));

    // Reset mock state
    parseJsonCalled = false;
    cliInput.parsedType = parseError;

    settingsManagerInit(&settingsManager, mock_readSettingsFromFlash_Matching);
}

void mock_readSettingsFromFlash_OldVersion(char buffer[], uint32_t length) {
    snprintf(buffer, length, "{\"modeCount\":5}");
}

void test_SettingsManagerInit_MergesDefaults_WhenNewFieldMissing(void) {
    SettingsManager settingsManager;
    memset(&settingsManager, 0, sizeof(SettingsManager));

    // Reset mock state
    parseJsonCalled = false;

    // Simulate successful parse of the OLD version
    // The parser would see "modeCount":5, but miss "equationEvalIntervalMs".
    // So it would produce a struct with modeCount=5 and equationEvalIntervalMs=DEFAULT(20).
    cliInput.parsedType = parseWriteSettings;
    cliInput.settings.modeCount = 5;
    cliInput.settings.equationEvalIntervalMs = 20;  // Default

    settingsManagerInit(&settingsManager, mock_readSettingsFromFlash_OldVersion);

    // Verify loaded settings are correct (merged)
    TEST_ASSERT_EQUAL_UINT8(5, settingsManager.currentSettings.modeCount);
    TEST_ASSERT_EQUAL_UINT8(20, settingsManager.currentSettings.equationEvalIntervalMs);
}

void test_SettingsJson_KeysMatchMacroCount(void) {
    char buf[PAGE_SECTOR];

    // getSettingsDefaultsJson returns a valid JSON object: {...}
    getSettingsDefaultsJson(buf, sizeof(buf));

    // Parse JSON
    lwjson_token_t tokens[128];
    lwjson_t lwjson;
    lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));
    TEST_ASSERT_EQUAL(lwjsonOK, lwjson_parse(&lwjson, buf));

    // The root token is the object itself
    TEST_ASSERT_EQUAL(LWJSON_TYPE_OBJECT, lwjson.first_token.type);

    // Count keys in root object
    int keyCount = 0;
    for (const lwjson_token_t *child = lwjson.first_token.u.first_child; child != NULL; child = child->next) {
        keyCount++;
    }



    int macroCount = 0;
#define X_COUNT(type, name, def) macroCount++;
    CHIP_SETTINGS_MAP(X_COUNT)
#undef X_COUNT

    TEST_ASSERT_EQUAL_MESSAGE(macroCount, keyCount, "JSON key count mismatch");

    lwjson_free(&lwjson);
}

void test_generateSettingsResponse_WithSettings(void) {
    char buffer[1024];
    const char *settings = "{\"foo\":1}";

    getSettingsResponse(buffer, sizeof(buffer), settings);

    // 1. Verify full string content
    char defaultsBuf[256];
    getSettingsDefaultsJson(defaultsBuf, sizeof(defaultsBuf));

    char expected[1024];
    sprintf(expected, "{\"settings\":%s,\"defaults\":%s}\n", settings, defaultsBuf);

    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    // 2. Verify it is valid JSON
    lwjson_token_t tokens[128];
    lwjson_t lwjson;
    lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));

    // Remove newline for parser if necessary, but lwjson might handle it.
    // Let's try parsing as is.
    TEST_ASSERT_EQUAL(lwjsonOK, lwjson_parse(&lwjson, buffer));

    const lwjson_token_t *t = lwjson_find(&lwjson, "settings");
    TEST_ASSERT_NOT_NULL(t);

    t = lwjson_find(&lwjson, "defaults");
    TEST_ASSERT_NOT_NULL(t);

    lwjson_free(&lwjson);
}

void test_generateSettingsResponse_NullSettings(void) {
    char buffer[1024];

    getSettingsResponse(buffer, sizeof(buffer), NULL);

    // 1. Verify full string content
    char defaultsBuf[256];
    getSettingsDefaultsJson(defaultsBuf, sizeof(defaultsBuf));

    char expected[1024];
    sprintf(expected, "{\"settings\":null,\"defaults\":%s}\n", defaultsBuf);

    TEST_ASSERT_EQUAL_STRING(expected, buffer);

    // 2. Verify it is valid JSON
    lwjson_token_t tokens[128];
    lwjson_t lwjson;
    lwjson_init(&lwjson, tokens, LWJSON_ARRAYSIZE(tokens));

    TEST_ASSERT_EQUAL(lwjsonOK, lwjson_parse(&lwjson, buffer));

    const lwjson_token_t *t = lwjson_find(&lwjson, "settings");
    TEST_ASSERT_NOT_NULL(t);
    TEST_ASSERT_EQUAL(LWJSON_TYPE_NULL, t->type);

    t = lwjson_find(&lwjson, "defaults");
    TEST_ASSERT_NOT_NULL(t);

    lwjson_free(&lwjson);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_SettingsJson_KeysMatchMacroCount);
    RUN_TEST(test_SettingsManagerInit_DoesNotWriteFlash_WhenFlashMatches);
    RUN_TEST(test_SettingsManagerInit_MergesDefaults_WhenNewFieldMissing);
    RUN_TEST(test_SettingsManagerInit_SetsDefaults);
    RUN_TEST(test_UpdateSettings_UpdatesChipStateSettings);
    RUN_TEST(test_generateSettingsResponse_NullSettings);
    RUN_TEST(test_generateSettingsResponse_WithSettings);
    return UNITY_END();
}
