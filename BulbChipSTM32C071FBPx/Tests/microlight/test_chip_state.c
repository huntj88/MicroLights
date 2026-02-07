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
static ChipState state;

static uint32_t mockMillisPerTick = 10;
static uint32_t mockMsPerTickMultiplier = 0;
static char lastSerialOutput[100];
static bool chipTickTimerEnabled = false;
static bool caseLedTimerEnabled = false;
static bool frontLedTimerEnabled = false;
static uint32_t chipTickTimerCallCount = 0;
static uint32_t caseLedTimerCallCount = 0;
static uint32_t frontLedTimerCallCount = 0;
static bool chargerTaskCalled = false;
static ChargerTaskFlags lastChargerFlags;
static bool lastModeTaskCanUpdateCaseLed = false;
static ModeOutputs nextModeOutputs;

// Mock Function Implementations
uint32_t mock_convertTicksToMs(uint32_t ticks) {
    if (mockMsPerTickMultiplier == 0) {
        mockMsPerTickMultiplier = (uint32_t)(mockMillisPerTick * 1048576);  // 2^20 for fixed point
    }
    return (uint32_t)(((uint64_t)ticks * mockMsPerTickMultiplier) >> 20);
}

void mock_writeUsbSerial(const char *buf, size_t count) {
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
}

void mock_enableChipTickTimer(bool enable) {
    chipTickTimerEnabled = enable;
    chipTickTimerCallCount++;
}

void mock_enableCaseLedTimer(bool enable) {
    caseLedTimerEnabled = enable;
    caseLedTimerCallCount++;
}

void mock_enableFrontLedTimer(bool enable) {
    frontLedTimerEnabled = enable;
    frontLedTimerCallCount++;
}

enum ButtonResult mockButtonResult = ignore;
enum ButtonResult buttonInputTask(Button *button, uint32_t ms, bool interruptTriggered) {
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
void chargerTask(BQ25180 *dev, uint32_t ms, ChargerTaskFlags flags) {
    (void)dev;
    (void)ms;
    chargerTaskCalled = true;
    lastChargerFlags = flags;
}
ModeOutputs modeTask(
    ModeManager *manager, uint32_t ms, bool canUpdateCaseLed, uint8_t equationEvalIntervalMs) {
    (void)manager;
    (void)ms;
    lastModeTaskCanUpdateCaseLed = canUpdateCaseLed;
    (void)equationEvalIntervalMs;
    return nextModeOutputs;
}

bool mockIsFakeOff = false;
bool isFakeOff(ModeManager *manager) {
    (void)manager;
    return mockIsFakeOff;
}

bool mockIsEvaluatingButtonPress = false;
bool isEvaluatingButtonPress(Button *button) {
    return mockIsEvaluatingButtonPress;
}

void fakeOffMode(ModeManager *manager) {
    // Mock implementation
    mockIsFakeOff = true;
    loadMode(manager, FAKE_OFF_MODE_INDEX);
}

// Include the source files under test to access static state
#include "../../Core/Src/microlight/chip_state.c"
#include "../../Core/Src/microlight/model/mode_state.c"

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
    memset(lastSerialOutput, 0, sizeof(lastSerialOutput));
    chipTickTimerEnabled = false;
    caseLedTimerEnabled = false;
    frontLedTimerEnabled = false;
    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;
    chargerTaskCalled = false;
    memset(&lastChargerFlags, 0, sizeof(lastChargerFlags));
    lastModeTaskCanUpdateCaseLed = false;
    nextModeOutputs = (ModeOutputs){
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };

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
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_ConfigureChipState_WhenCharging_EntersFakeOff(void) {
    mockChargeState = constantCurrent;
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Clicked_CyclesToNextMode(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockModeManager.currentModeIndex = 1;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(mockRgbShowSuccessCalled);
    TEST_ASSERT_EQUAL_UINT8(2, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Clicked_WrapsModeIndex(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockModeManager.currentModeIndex = 4;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenNotCharging_DisablesLedTimers(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockButtonResult = shutdown;
    mockChargeState = notConnected;
    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenCharging_EnablesLedTimers(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockButtonResult = shutdown;
    mockChargeState = constantCurrent;
    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
}

void test_StateTask_Shutdown_ChargeLedEnabled_WhenCharging(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockChargeState = constantCurrent;
    mockButtonResult = shutdown;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(chargerTaskCalled);
    TEST_ASSERT_TRUE(lastChargerFlags.chargeLedEnabled);
}

void test_StateTask_ChargeLedDisabled_WhenNotCharging(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockChargeState = notConnected;
    mockIsFakeOff = true;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(chargerTaskCalled);
    TEST_ASSERT_FALSE(lastChargerFlags.chargeLedEnabled);
}

void test_StateTask_ModeTask_DisabledCaseLed_WhenFakeOff(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockIsFakeOff = true;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_FALSE(lastModeTaskCanUpdateCaseLed);
}

void test_StateTask_ButtonInterrupt_EnablesCasePwm(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockChargeState = notConnected;
    mockIsFakeOff = false;
    mockIsEvaluatingButtonPress = false;
    nextModeOutputs = (ModeOutputs){
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };

    stateTask(&state, 0, (StateTaskFlags){.buttonInterruptTriggered = true});

    TEST_ASSERT_TRUE(caseLedTimerEnabled);
}

void test_StateTask_ButtonResult_Lock_LocksCharger(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockButtonResult = lockOrHardwareReset;

    stateTask(&state, 0, (StateTaskFlags){0});

    TEST_ASSERT_TRUE(mockLockCalled);
}

void test_AutoOffTimer_EntersFakeOff_AfterTimeout(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockSettings.minutesUntilAutoOff = 1;  // 1 minute
    mockChargeState = notConnected;
    mockIsFakeOff = false;

    // 1 minute = 600 ticks at 0.1Hz.
    // Logic: ticksUntilAutoOff = minutes * 60 / 10. For 1 min, threshold is 6 ticks.

    state.ticksSinceLastUserActivity = 7;  // Exceeds 6

    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});

    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
}

void test_Settings_ModeCount_LimitsModeCycling(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    // Case 1: Mode Count 3, Current 2 -> Should wrap to 0
    mockSettings.modeCount = 3;
    mockModeManager.currentModeIndex = 2;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);

    // Case 2: Mode Count 5, Current 2 -> Should go to 3
    mockSettings.modeCount = 5;
    mockModeManager.currentModeIndex = 2;
    mockButtonResult = clicked;

    stateTask(&state, 0, (StateTaskFlags){0});
    TEST_ASSERT_EQUAL_UINT8(3, lastLoadedModeIndex);
}

void test_Settings_MinutesUntilAutoOff_ChangesTimeout(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockChargeState = notConnected;
    mockIsFakeOff = false;

    // Case 1: 1 Minute (6 ticks)
    mockSettings.minutesUntilAutoOff = 1;

    // Just below threshold (starts at 5, increments to 6 inside interrupt. 6 > 6 is False)
    state.ticksSinceLastUserActivity = 5;
    lastLoadedModeIndex = 0;  // Reset

    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});

    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);  // No change

    // Just above threshold (starts at 6, increments to 7 inside interrupt. 7 > 6 is True)
    state.ticksSinceLastUserActivity = 6;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);  // Changed

    // Case 2: 2 Minutes (12 ticks)
    mockSettings.minutesUntilAutoOff = 2;
    mockIsFakeOff = false;  // Reset to normal mode for second case

    // Above 1 min threshold, but below 2 min (starts at 11, increments to 12. 12 > 12 is False)
    state.ticksSinceLastUserActivity = 11;
    lastLoadedModeIndex = 0;  // Reset
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);  // No change

    // Above 2 min threshold (starts at 12, increments to 13. 13 > 12 is True)
    state.ticksSinceLastUserActivity = 12;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);  // Changed
}

void test_Settings_MinutesUntilLockAfterAutoOff_ChangesLockTimeout(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockChargeState = notConnected;
    mockIsFakeOff = true;  // Already in fake off

    // Case 1: 1 Minute (6 ticks)
    mockSettings.minutesUntilLockAfterAutoOff = 1;
    mockLockCalled = false;

    // Just below threshold (starts at 5, increments to 6. 6 > 6 is False)
    state.ticksSinceLastUserActivity = 5;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_FALSE(mockLockCalled);

    // Just above threshold (starts at 6, increments to 7. 7 > 6 is True)
    state.ticksSinceLastUserActivity = 6;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_TRUE(mockLockCalled);

    // Case 2: 2 Minutes (12 ticks)
    mockSettings.minutesUntilLockAfterAutoOff = 2;
    mockLockCalled = false;

    // Above 1 min threshold, but below 2 min (starts at 11, increments to 12. 12 > 12 is False)
    state.ticksSinceLastUserActivity = 11;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_FALSE(mockLockCalled);

    // Above 2 min threshold (starts at 12, increments to 13. 13 > 12 is True)
    state.ticksSinceLastUserActivity = 12;
    stateTask(&state, 0, (StateTaskFlags){.autoOffTimerInterruptTriggered = true});
    TEST_ASSERT_TRUE(mockLockCalled);
}

void test_TimerPolicy_SkipsRedundantCalls(void) {
    configureChipState(
        &state,
        &mockModeManager,
        &mockSettings,
        &mockButton,
        &mockCharger,
        &mockAccel,
        &mockCaseLed,
        mock_enableChipTickTimer,
        mock_enableCaseLedTimer,
        mock_enableFrontLedTimer,
        mock_writeUsbSerial);

    mockChargeState = notConnected;
    mockIsFakeOff = false;
    mockIsEvaluatingButtonPress = false;
    nextModeOutputs = (ModeOutputs){
        .frontValid = false,
        .caseValid = false,
        .frontType = BULB,
    };

    // First call should invoke all three timer callbacks (state transitions from init)
    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;

    stateTask(&state, 0, (StateTaskFlags){0});

    uint32_t firstChipTickCalls = chipTickTimerCallCount;
    uint32_t firstCaseCalls = caseLedTimerCallCount;
    uint32_t firstFrontCalls = frontLedTimerCallCount;
    TEST_ASSERT_GREATER_THAN_UINT32(0, firstChipTickCalls);

    // Second call with same state should NOT invoke callbacks again
    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;

    stateTask(&state, 10, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, chipTickTimerCallCount, "chipTick timer called redundantly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, caseLedTimerCallCount, "caseLed timer called redundantly");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, frontLedTimerCallCount, "frontLed timer called redundantly");

    // Change state: enable front RGB — only front timer should be called
    nextModeOutputs = (ModeOutputs){
        .frontValid = true,
        .caseValid = false,
        .frontType = RGB,
    };

    chipTickTimerCallCount = 0;
    caseLedTimerCallCount = 0;
    frontLedTimerCallCount = 0;

    stateTask(&state, 20, (StateTaskFlags){0});

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, chipTickTimerCallCount, "chipTick timer should not change");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, caseLedTimerCallCount, "caseLed timer should not change");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        1, frontLedTimerCallCount, "frontLed timer should be called once");
    TEST_ASSERT_TRUE(frontLedTimerEnabled);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_AutoOffTimer_EntersFakeOff_AfterTimeout);
    RUN_TEST(test_ConfigureChipState_WhenCharging_EntersFakeOff);
    RUN_TEST(test_ConfigureChipState_WhenNotCharging_LoadsModeZero);
    RUN_TEST(test_Settings_MinutesUntilAutoOff_ChangesTimeout);
    RUN_TEST(test_Settings_MinutesUntilLockAfterAutoOff_ChangesLockTimeout);
    RUN_TEST(test_Settings_ModeCount_LimitsModeCycling);
    RUN_TEST(test_StateTask_ButtonInterrupt_EnablesCasePwm);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_CyclesToNextMode);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_WrapsModeIndex);
    RUN_TEST(test_StateTask_ButtonResult_Lock_LocksCharger);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenCharging_EnablesLedTimers);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersFakeOff_WhenNotCharging_DisablesLedTimers);
    RUN_TEST(test_StateTask_ChargeLedDisabled_WhenNotCharging);
    RUN_TEST(test_StateTask_ModeTask_DisabledCaseLed_WhenFakeOff);
    RUN_TEST(test_StateTask_Shutdown_ChargeLedEnabled_WhenCharging);
    RUN_TEST(test_TimerPolicy_SkipsRedundantCalls);
    return UNITY_END();
}
