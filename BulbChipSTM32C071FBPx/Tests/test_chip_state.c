#include "unity.h"
#include <string.h>
#include <stdbool.h>

// Include headers required by chip_state.c
#include "chip_state.h"
#include "mode_manager.h"
#include "settings_manager.h"
#include "device/button.h"
#include "device/bq25180.h"
#include "device/mc3479.h"
#include "device/rgb_led.h"

// Mock Data
static ModeManager mockModeManager;
static ChipSettings mockSettings;
static Button mockButton;
static BQ25180 mockCharger;
static MC3479 mockAccel;
static RGBLed mockCaseLed;

static uint8_t lastWrittenBulbState = 255;
static float mockMillisPerTick = 10.0f;
static bool ledTimersStarted = false;
static bool ledTimersStopped = false;
static char lastSerialOutput[100];

// Mock Function Implementations
void mock_writeBulbLedPin(uint8_t state) {
    lastWrittenBulbState = state;
}

float mock_getMillisecondsPerChipTick() {
    return mockMillisPerTick;
}

void mock_startLedTimers() {
    ledTimersStarted = true;
}

void mock_stopLedTimers() {
    ledTimersStopped = true;
}

void mock_writeUsbSerial(uint8_t itf, const char *buf, uint32_t count) {
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

enum ButtonResult mockButtonResult = ignore;
enum ButtonResult buttonInputTask(Button *button, uint16_t ms) {
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

void rgbTask(RGBLed *led, uint16_t ms) {}
void mc3479Task(MC3479 *dev, uint16_t ms) {}
void chargerTask(BQ25180 *dev, uint16_t ms, bool unplugLockEnabled, bool chargeLedEnabled) {}

bool mockIsFakeOff = false;
bool isFakeOff(ModeManager *manager) {
    return mockIsFakeOff;
}

bool mockIsEvaluatingButtonPress = false;
bool isEvaluatingButtonPress(Button *button) {
    return mockIsEvaluatingButtonPress;
}

bool mockIsOverThreshold = false;
bool isOverThreshold(MC3479 *dev, float threshold) {
    return mockIsOverThreshold;
}

uint8_t lastRgbR, lastRgbG, lastRgbB;
void rgbShowUserColor(RGBLed *led, uint8_t r, uint8_t g, uint8_t b) {
    lastRgbR = r;
    lastRgbG = g;
    lastRgbB = b;
}

// Include the source file under test to access static state
#include "../Core/Src/chip_state.c"

// Setup and Teardown
void setUp(void) {
    memset(&mockModeManager, 0, sizeof(ModeManager));
    memset(&mockSettings, 0, sizeof(ChipSettings));
    memset(&mockButton, 0, sizeof(Button));
    memset(&mockCharger, 0, sizeof(BQ25180));
    memset(&mockAccel, 0, sizeof(MC3479));
    memset(&mockCaseLed, 0, sizeof(RGBLed));
    
    lastWrittenBulbState = 255;
    mockMillisPerTick = 10.0f;
    ledTimersStarted = false;
    ledTimersStopped = false;
    memset(lastSerialOutput, 0, sizeof(lastSerialOutput));
    
    mockChargeState = notConnected;
    lastLoadedModeIndex = 255;
    mockButtonResult = ignore;
    mockRgbShowSuccessCalled = false;
    mockLockCalled = false;
    mockIsFakeOff = false;
    mockIsEvaluatingButtonPress = false;
    mockIsOverThreshold = false;
    lastRgbR = 0; lastRgbG = 0; lastRgbB = 0;
    
    state = (ChipState){0}; // Reset internal state
}

void tearDown(void) {}

// Test Cases

void test_ConfigureChipState_WhenNotCharging_LoadsModeZero(void) {
    mockChargeState = notConnected;
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_ConfigureChipState_WhenCharging_EntersFakeOff(void) {
    mockChargeState = constantCurrent;
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
    TEST_ASSERT_TRUE(ledTimersStarted);
}

void test_StateTask_ButtonResult_Clicked_CyclesToNextMode(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    mockModeManager.currentModeIndex = 1;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;
    
    stateTask();
    
    TEST_ASSERT_TRUE(mockRgbShowSuccessCalled);
    TEST_ASSERT_EQUAL_UINT8(2, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Clicked_WrapsModeIndex(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    mockModeManager.currentModeIndex = 4;
    mockSettings.modeCount = 5;
    mockButtonResult = clicked;
    
    stateTask();
    
    TEST_ASSERT_EQUAL_UINT8(0, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Shutdown_EntersFakeOff(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    mockButtonResult = shutdown;
    
    stateTask();
    
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
}

void test_StateTask_ButtonResult_Lock_LocksCharger(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    mockButtonResult = lockOrHardwareReset;
    
    stateTask();
    
    TEST_ASSERT_TRUE(mockLockCalled);
}

void test_UpdateMode_FrontLed_FollowsSimplePattern(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    // Setup a simple pattern: High at 0ms, Low at 500ms
    mockModeManager.currentMode.has_front = true;
    mockModeManager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.front.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt_count = 2;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = high;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[1].ms = 500;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[1].output.type = BULB;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[1].output.data.bulb = low;

    // Test at 100ms (should be High)
    state.chipTick = 10; // 10 * 10ms = 100ms
    chipTickInterrupt(); // Calls updateMode
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    // Test at 600ms (should be Low)
    state.chipTick = 60; // 60 * 10ms = 600ms
    chipTickInterrupt();
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_AutoOffTimer_EntersFakeOff_AfterTimeout(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    mockSettings.minutesUntilAutoOff = 1; // 1 minute
    mockChargeState = notConnected;
    mockIsFakeOff = false;
    
    // 1 minute = 600 ticks at 0.1Hz (10s per tick? No, 0.1Hz is 10s period? No, 0.1Hz is 1 tick per 10s. Wait.
    // Code says: 12 megahertz / 65535 / 1831 = 0.1 hz. So 1 tick every 10 seconds.
    // Code: uint16_t ticksUntilAutoOff = state.settings->minutesUntilAutoOff * 60 / 10;
    // If minutes = 1, ticks = 6. 6 * 10s = 60s. Correct.
    
    state.ticksSinceLastUserActivity = 7; // Exceeds 6
    
    autoOffTimerInterrupt();
    
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, lastLoadedModeIndex);
}

void test_UpdateMode_CaseLed_FollowsSimplePattern(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    // Setup a simple pattern for Case LED
    mockModeManager.currentMode.has_case_comp = true;
    mockModeManager.currentMode.case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.case_comp.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.type = RGB;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.r = 255;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.b = 128;

    mockIsEvaluatingButtonPress = false;

    // Test at 100ms
    state.chipTick = 10; // 100ms
    chipTickInterrupt(); // Calls updateMode
    
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(128, lastRgbB);
}

void test_UpdateMode_CaseLed_Off_WhenNoPattern(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    mockModeManager.currentMode.has_case_comp = false;
    mockIsEvaluatingButtonPress = false;
    
    // Set last RGB to something else
    lastRgbR = 10; lastRgbG = 10; lastRgbB = 10;

    state.chipTick = 10;
    chipTickInterrupt();
    
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_UpdateMode_CaseLed_NotUpdated_WhenButtonEvaluating(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    // Setup pattern
    mockModeManager.currentMode.has_case_comp = true;
    mockModeManager.currentMode.case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.case_comp.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.type = RGB;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.r = 255;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.g = 255;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.b = 255;

    mockIsEvaluatingButtonPress = true;
    
    // Set last RGB to something else (e.g. from button press logic, though we are mocking)
    lastRgbR = 50; lastRgbG = 50; lastRgbB = 50;

    state.chipTick = 10;
    chipTickInterrupt();
    
    // Should NOT have updated to 255, 255, 255
    TEST_ASSERT_EQUAL_UINT8(50, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(50, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(50, lastRgbB);
}

void test_UpdateMode_CaseLed_FollowsSimplePatternMultipleChanges(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    // Setup a simple pattern for Case LED
    mockModeManager.currentMode.has_case_comp = true;
    mockModeManager.currentMode.case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.case_comp.pattern.data.simple.duration = 2000;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt_count = 3;
    
    // 0ms: Red
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.type = RGB;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.r = 255;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.b = 0;

    // 500ms: Green
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[1].ms = 500;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[1].output.type = RGB;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[1].output.data.rgb.r = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[1].output.data.rgb.g = 255;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[1].output.data.rgb.b = 0;

    // 1000ms: Blue
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[2].ms = 1000;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[2].output.type = RGB;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[2].output.data.rgb.r = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[2].output.data.rgb.g = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[2].output.data.rgb.b = 255;

    mockIsEvaluatingButtonPress = false;

    // Test at 100ms (Should be Red)
    state.chipTick = 10; // 100ms
    chipTickInterrupt();
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Test at 600ms (Should be Green)
    state.chipTick = 60; // 600ms
    chipTickInterrupt();
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Test at 1500ms (Should be Blue)
    state.chipTick = 150; // 1500ms
    chipTickInterrupt();
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbB);

    // Test at 2100ms (Should be Red - loop back to 100ms)
    state.chipTick = 210; // 2100ms % 2000ms = 100ms
    chipTickInterrupt();
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_UpdateMode_AccelTrigger_OverridesPatterns_WhenThresholdMet(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    // Setup Default Mode: Front OFF, Case OFF
    mockModeManager.currentMode.has_front = true;
    mockModeManager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.front.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = low;

    mockModeManager.currentMode.has_case_comp = true;
    mockModeManager.currentMode.case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.case_comp.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.type = RGB;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.r = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.b = 0;

    // Setup Accel Trigger: Front ON, Case RED
    mockModeManager.currentMode.has_accel = true;
    mockModeManager.currentMode.accel.triggers_count = 1;
    mockModeManager.currentMode.accel.triggers[0].threshold = 1000; // Not used by logic yet, but good to set
    
    mockModeManager.currentMode.accel.triggers[0].has_front = true;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.type = BULB;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.data.bulb = high;

    mockModeManager.currentMode.accel.triggers[0].has_case_comp = true;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.data.simple.changeAt[0].output.type = RGB;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.data.simple.changeAt[0].output.data.rgb.r = 255;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    mockModeManager.currentMode.accel.triggers[0].case_comp.pattern.data.simple.changeAt[0].output.data.rgb.b = 0;

    // Trigger the accel
    mockIsOverThreshold = true;

    state.chipTick = 10;
    chipTickInterrupt();

    // Should be High and Red
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_UpdateMode_AccelTrigger_DoesNotOverride_WhenThresholdNotMet(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    // Setup Default Mode: Front OFF
    mockModeManager.currentMode.has_front = true;
    mockModeManager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.front.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = low;

    // Setup Accel Trigger: Front ON
    mockModeManager.currentMode.has_accel = true;
    mockModeManager.currentMode.accel.triggers_count = 1;
    mockModeManager.currentMode.accel.triggers[0].has_front = true;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.type = BULB;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.data.bulb = high;

    // Do NOT trigger the accel
    mockIsOverThreshold = false;

    state.chipTick = 10;
    chipTickInterrupt();

    // Should be Low (Default)
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_UpdateMode_AccelTrigger_PartialOverride(void) {
    configureChipState(&mockModeManager, &mockSettings, &mockButton, &mockCharger, &mockAccel, &mockCaseLed, 
                       mock_writeUsbSerial, mock_writeBulbLedPin, mock_getMillisecondsPerChipTick, 
                       mock_startLedTimers, mock_stopLedTimers);
    
    // Setup Default Mode: Front OFF, Case BLUE
    mockModeManager.currentMode.has_front = true;
    mockModeManager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.front.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    mockModeManager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = low;

    mockModeManager.currentMode.has_case_comp = true;
    mockModeManager.currentMode.case_comp.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.case_comp.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.type = RGB;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.r = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    mockModeManager.currentMode.case_comp.pattern.data.simple.changeAt[0].output.data.rgb.b = 255;

    // Setup Accel Trigger: Front ON, NO Case override
    mockModeManager.currentMode.has_accel = true;
    mockModeManager.currentMode.accel.triggers_count = 1;
    
    mockModeManager.currentMode.accel.triggers[0].has_front = true;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.duration = 1000;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt_count = 1;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].ms = 0;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.type = BULB;
    mockModeManager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.data.bulb = high;

    mockModeManager.currentMode.accel.triggers[0].has_case_comp = false; // No override

    // Trigger the accel
    mockIsOverThreshold = true;

    state.chipTick = 10;
    chipTickInterrupt();

    // Should be High (Triggered) and Blue (Default)
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbB);
}


int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ConfigureChipState_WhenNotCharging_LoadsModeZero);
    RUN_TEST(test_ConfigureChipState_WhenCharging_EntersFakeOff);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_CyclesToNextMode);
    RUN_TEST(test_StateTask_ButtonResult_Clicked_WrapsModeIndex);
    RUN_TEST(test_StateTask_ButtonResult_Shutdown_EntersFakeOff);
    RUN_TEST(test_StateTask_ButtonResult_Lock_LocksCharger);
    RUN_TEST(test_UpdateMode_FrontLed_FollowsSimplePattern);
    RUN_TEST(test_AutoOffTimer_EntersFakeOff_AfterTimeout);
    RUN_TEST(test_UpdateMode_CaseLed_FollowsSimplePattern);
    RUN_TEST(test_UpdateMode_CaseLed_Off_WhenNoPattern);
    RUN_TEST(test_UpdateMode_CaseLed_NotUpdated_WhenButtonEvaluating);
    RUN_TEST(test_UpdateMode_CaseLed_FollowsSimplePatternMultipleChanges);
    RUN_TEST(test_UpdateMode_AccelTrigger_OverridesPatterns_WhenThresholdMet);
    RUN_TEST(test_UpdateMode_AccelTrigger_DoesNotOverride_WhenThresholdNotMet);
    RUN_TEST(test_UpdateMode_AccelTrigger_PartialOverride);
    return UNITY_END();
}
