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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_SettingsManagerInit_DoesNotWriteFlash_WhenFlashMatches);
    RUN_TEST(test_SettingsManagerInit_MergesDefaults_WhenNewFieldMissing);
    RUN_TEST(test_SettingsManagerInit_SetsDefaults);
    RUN_TEST(test_UpdateSettings_UpdatesChipStateSettings);
    return UNITY_END();
}
