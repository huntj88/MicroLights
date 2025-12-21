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
    return UNITY_END();
}
