#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "unity.h"

// Include headers required by chip_state.c
#include "microlight/chip_state.h"
#include "microlight/device/bq25180.h"
#include "microlight/device/button.h"
#include "microlight/device/mc3479.h"
#include "microlight/device/rgb_led.h"
#include "microlight/mode_manager.h"
#include "microlight/settings_manager.h"

// Mock Data
static ModeManager mockModeManager;
static ChipSettings mockSettings;
static Button mockButton;
static BQ25180 mockCharger;
static MC3479 mockAccel;
static RGBLed mockCaseLed;

static uint32_t mockMillisPerTick = 10;
static uint32_t mockMsPerTickMultiplier = 0;
static bool ledTimersStarted = false;
static char lastSerialOutput[100];

// Mock Function Implementations
uint32_t mock_convertTicksToMs(uint32_t ticks) {
    if (mockMsPerTickMultiplier == 0) {
        mockMsPerTickMultiplier = (uint32_t)(mockMillisPerTick * 1048576);  // 2^20 for fixed point
    }
    return (uint32_t)(((uint64_t)ticks * mockMsPerTickMultiplier) >> 20);
}

void mock_writeUsbSerial(const char *buf, uint32_t count) {
    strncpy(lastSerialOutput, buf, count);
    lastSerialOutput[count] = '\0';
}

// External Mocks (Functions called by chip_state.c)
enum ChargeState mockChargeState = notConnected;
enum ChargeState getChargingState(BQ25180 *dev) {
    return mockChargeState;
}

uint8_t lastLoadedModeIndex = 255;
void loadMode(ModeManager *manager, uint8_t index) {
    lastLoadedModeIndex = index;
    manager->currentModeIndex = index;
    ledTimersStarted = true;
}

enum ButtonResult mockButtonResult = ignore;
enum ButtonResult buttonInputTask(Button *button, uint32_t ms) {
    return mockButtonResult;
}

bool mockRgbShowSuccessCalled = false;
void rgbShowSuccess(RGBLed *led) {
    mockRgbShowSuccessCalled = true;
}

bool mockLockCalled = false;
void lock(BQ25180 *dev) {
    mockLockCalled = true;
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

bool mockIsFakeOff = false;
bool isFakeOff(ModeManager *manager) {
    return mockIsFakeOff;
}

bool mockIsEvaluatingButtonPress = false;
bool isEvaluatingButtonPress(Button *button) {
    return mockIsEvaluatingButtonPress;
}

void fakeOffMode(ModeManager *manager, bool enableLedTimers) {
    // Mock implementation
    loadMode(manager, FAKE_OFF_MODE_INDEX);
    ledTimersStarted = enableLedTimers;
}

// Include the source files under test to access static state
#include "../Core/Src/microlight/chip_state.c"
#include "../Core/Src/microlight/model/mode_state.c"

// Setup and Teardown
void setUp(void) {
    memset(&mockModeManager, 0, sizeof(ModeManager));
    memset(&mockSettings, 0, sizeof(ChipSettings));
    memset(&mockButton, 0, sizeof(Button));
    memset(&mockCharger, 0, sizeof(BQ25180));
    memset(&mockAccel, 0, sizeof(MC3479));
    memset(&mockCaseLed, 0, sizeof(RGBLed));

    mockMillisPerTick = 10.0f;
    mockMsPerTickMultiplier = 0;
    ledTimersStarted = false;
    memset(lastSerialOutput, 0, sizeof(lastSerialOutput));

    mockChargeState = notConnected;
    lastLoadedModeIndex = 255;
    mockButtonResult = ignore;
    mockRgbShowSuccessCalled = false;
    mockLockCalled = false;
    mockIsFakeOff = false;
    mockIsEvaluatingButtonPress = false;

    state = (ChipState){0};  // Reset internal state
}

void tearDown(void) {
}

// Test Cases

void test_ConfigureChipState_WhenNotCharging_LoadsModeZero(void) {
    mockChargeState = notConnected;
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_ConfigureChipState_WhenCharging_EntersFakeOff(void) {
    mockChargeState = constantCurrent;
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
    TEST_ASSERT_TRUE(ledTimersStarted);
}

void test_StateTask_ButtonResult_Clicked_CyclesToNextMode(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockModeManager.currentModeIndex = 1;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;

    stateTask();

    TEST_ASSERT_TRUE(mockRgbShowSuccessCalled);
    TEST_ASSERT_EQUAL_UINT8(2, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Clicked_WrapsModeIndex(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockModeManager.currentModeIndex = 4;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;

    stateTask();

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenNotCharging_DisablesLedTimers(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockButtonResult = shutdown;
    mockChargeState = notConnected;
    ledTimersStarted = true;

    stateTask();

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
    TEST_ASSERT_FALSE(ledTimersStarted);
}

void test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenCharging_EnablesLedTimers(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockButtonResult = shutdown;
    mockChargeState = constantCurrent;
    ledTimersStarted = false;

    stateTask();

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
    TEST_ASSERT_TRUE(ledTimersStarted);
}

void test_StateTask_ButtonResult_Lock_LocksCharger(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockButtonResult = lockOrHardwareReset;

    stateTask();

    TEST_ASSERT_TRUE(mockLockCalled);
}

void test_AutoOffTimer_EntersFakeOff_AfterTimeout(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockSettings.minutesUntilAutoOff = 1;  // 1 minute
    mockChargeState = notConnected;
    mockIsFakeOff = false;

    // 1 minute = 600 ticks at 0.1Hz (10s per tick? No, 0.1Hz is 10s period? No, 0.1Hz is 1 tick per
    // 10s. Wait. Code says: 12 megahertz / 65535 / 1831 = 0.1 hz. So 1 tick every 10 seconds. Code:
    // uint16_t ticksUntilAutoOff = state.settings->minutesUntilAutoOff * 60 / 10; If minutes = 1,
    // ticks = 6. 6 * 10s = 60s. Correct.

    state.ticksSinceLastUserActivity = 7;  // Exceeds 6

    autoOffTimerInterrupt();

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
    TEST_ASSERT_FALSE(ledTimersStarted);
}

void test_Settings_ModeCount_LimitsModeCycling(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    // Case 1: Mode Count 3, Current 2 -> Should wrap to 0
    mockSettings.modeCount = 3;
    mockModeManager.currentModeIndex = 2;
    mockButtonResult = clicked;

    stateTask();
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);

    // Case 2: Mode Count 5, Current 2 -> Should go to 3
    mockSettings.modeCount = 5;
    mockModeManager.currentModeIndex = 2;
    mockButtonResult = clicked;

    stateTask();
    TEST_ASSERT_EQUAL_UINT8(3, lastLoadedModeIndex);
}

void test_Settings_MinutesUntilAutoOff_ChangesTimeout(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockChargeState = notConnected;
    mockIsFakeOff = false;

    // Case 1: 1 Minute (6 ticks)
    mockSettings.minutesUntilAutoOff = 1;

    // Just below threshold (starts at 5, increments to 6 inside interrupt. 6 > 6 is False)
    state.ticksSinceLastUserActivity = 5;
    lastLoadedModeIndex = 0;  // Reset
    autoOffTimerInterrupt();
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);  // No change

    // Just above threshold (starts at 6, increments to 7 inside interrupt. 7 > 6 is True)
    state.ticksSinceLastUserActivity = 6;
    autoOffTimerInterrupt();
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);  // Changed

    // Case 2: 2 Minutes (12 ticks)
    mockSettings.minutesUntilAutoOff = 2;

    // Above 1 min threshold, but below 2 min (starts at 11, increments to 12. 12 > 12 is False)
    state.ticksSinceLastUserActivity = 11;
    lastLoadedModeIndex = 0;  // Reset
    autoOffTimerInterrupt();
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);  // No change

    // Above 2 min threshold (starts at 12, increments to 13. 13 > 12 is True)
    state.ticksSinceLastUserActivity = 12;
    autoOffTimerInterrupt();
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);  // Changed
}

void test_Settings_MinutesUntilLockAfterAutoOff_ChangesLockTimeout(void) {
    configureChipState(
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_writeUsbSerial,
        mock_convertTicksToMs);

    mockChargeState = notConnected;
    mockIsFakeOff = true;  // Already in fake off

    // Case 1: 1 Minute (6 ticks)
    mockSettings.minutesUntilLockAfterAutoOff = 1;
    mockLockCalled = false;

    // Just below threshold (starts at 5, increments to 6. 6 > 6 is False)
    state.ticksSinceLastUserActivity = 5;
    autoOffTimerInterrupt();
    TEST_ASSERT_FALSE(mockLockCalled);

    // Just above threshold (starts at 6, increments to 7. 7 > 6 is True)
    state.ticksSinceLastUserActivity = 6;
    autoOffTimerInterrupt();
    TEST_ASSERT_TRUE(mockLockCalled);

    // Case 2: 2 Minutes (12 ticks)
    mockSettings.minutesUntilLockAfterAutoOff = 2;
    mockLockCalled = false;

    // Above 1 min threshold, but below 2 min (starts at 11, increments to 12. 12 > 12 is False)
    state.ticksSinceLastUserActivity = 11;
    autoOffTimerInterrupt();
    TEST_ASSERT_FALSE(mockLockCalled);

    // Above 2 min threshold (starts at 12, increments to 13. 13 > 12 is True)
    state.ticksSinceLastUserActivity = 12;
    autoOffTimerInterrupt();
    TEST_ASSERT_TRUE(mockLockCalled);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_AutoOffTimer_EntersFakeOff_AfterTimeout);
    RUN_TEST(test_ConfigureChipState_WhenCharging_EntersFakeOff);
    RUN_TEST(test_ConfigureChipState_WhenNotCharging_LoadsModeZero);
    RUN_TEST(test_Settings_MinutesUntilAutoOff_ChangesTimeout);
    RUN_TEST(test_Settings_MinutesUntilLockAfterAutoOff_ChangesLockTimeout);
    RUN_TEST(test_Settings_ModeCount_LimitsModeCycling);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_CyclesToNextMode);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_WrapsModeIndex);
    RUN_TEST(test_StateTask_ButtonResult_Lock_LocksCharger);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenCharging_EnablesLedTimers);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenNotCharging_DisablesLedTimers);
    return UNITY_END();
}
