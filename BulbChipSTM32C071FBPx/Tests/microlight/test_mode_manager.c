#include <stdbool.h>
#include <string.h>
#include "unity.h"

#include "microlight/device/mc3479.h"
#include "microlight/json/command_parser.h"
#include "microlight/json/json_buf.h"
#include "microlight/json/mode_parser.h"
#include "microlight/mode_manager.h"
#include "microlight/model/cli_model.h"

static MC3479 mockAccel;
static RGBLed mockCaseLed;
static RGBLed mockFrontLed;
static bool accelEnabled = false;
static bool accelDisabled = false;
static uint8_t lastReadModeIndex = 255;
#define TEST_JSON_BUFFER_SIZE 2048
static uint8_t lastWrittenBulbState = 255;
static uint8_t lastRgbR, lastRgbG, lastRgbB;
static uint8_t lastFrontRgbR, lastFrontRgbG, lastFrontRgbB;
static uint8_t mockAccelMagnitude = 0;
static bool writeToSerialCalled = false;
static char lastSerialBuffer[256];
static size_t lastSerialCount = 0;

// Mock Functions
void mc3479Enable(MC3479 *dev) {
    accelEnabled = true;
}

void mc3479Disable(MC3479 *dev) {
    accelDisabled = true;
}

void mock_writeBulbLedPin(uint8_t state) {
    lastWrittenBulbState = state;
}

void rgbShowUserColor(RGBLed *led, uint8_t r, uint8_t g, uint8_t b) {
    if (led == &mockFrontLed) {
        lastFrontRgbR = r;
        lastFrontRgbG = g;
        lastFrontRgbB = b;
    } else {
        lastRgbR = r;
        lastRgbG = g;
        lastRgbB = b;
    }
}

bool isOverThreshold(MC3479 *dev, uint8_t threshold) {
    return mockAccelMagnitude > threshold;
}

void mock_writeToSerial(const char *buf, size_t count) {
    writeToSerialCalled = true;
    if (count >= sizeof(lastSerialBuffer)) {
        count = sizeof(lastSerialBuffer) - 1U;
    }
    memcpy(lastSerialBuffer, buf, count);
    lastSerialBuffer[count] = '\0';
    lastSerialCount = count;
}

void mock_readSavedMode(uint8_t mode, char buffer[], size_t length) {
    lastReadModeIndex = mode;
    if (mode == 1) {
        // Standard valid mode
        strcpy(
            buffer,
            "{\"command\":\"writeMode\",\"index\":1,\"mode\":{\"name\":\"test\",\"front\":{"
            "\"pattern\":{\"type\":\"simple\",\"name\":\"test\",\"duration\":1000,\"changeAt\":["
            "{\"ms\":0,\"output\":\"low\"}]}}}}");
    } else if (mode == 2) {
        // Mode with Accel
        strcpy(
            buffer,
            "{\"command\":\"writeMode\",\"index\":2,\"mode\":{\"name\":\"accel\",\"front\":{"
            "\"pattern\":{\"type\":\"simple\",\"name\":\"on\",\"duration\":100,\"changeAt\":[{"
            "\"ms\":0,\"output\":\"high\"}]}},\"accel\":{\"triggers\":[{\"threshold\":100,"
            "\"front\":{\"pattern\":{\"type\":\"simple\",\"name\":\"flash\",\"duration\":100,"
            "\"changeAt\":[{\"ms\":0,\"output\":\"low\"}]}}}]}}}");
    } else if (mode == 3) {
        // Mode without Accel
        strcpy(
            buffer,
            "{\"command\":\"writeMode\",\"index\":3,\"mode\":{\"name\":\"no_accel\",\"front\":{"
            "\"pattern\":{\"type\":\"simple\",\"name\":\"on\",\"duration\":100,\"changeAt\":[{"
            "\"ms\":0,\"output\":\"high\"}]}}}}");
    } else {
        // Default or empty
        strcpy(buffer, "");
    }
}

// Include source
char testJsonBuf[TEST_JSON_BUFFER_SIZE];
#include "../../Core/Src/microlight/mode_manager.c"
#include "../../Core/Src/microlight/model/mode_state.c"

void setUp(void) {
    memset(&mockAccel, 0, sizeof(MC3479));
    memset(&mockCaseLed, 0, sizeof(RGBLed));
    memset(&mockFrontLed, 0, sizeof(RGBLed));
    accelEnabled = false;
    accelDisabled = false;
    lastReadModeIndex = 255;
    lastWrittenBulbState = 255;
    lastRgbR = 0;
    lastRgbG = 0;
    lastRgbB = 0;
    lastFrontRgbR = 0;
    lastFrontRgbG = 0;
    lastFrontRgbB = 0;
    mockAccelMagnitude = 0;
    writeToSerialCalled = false;
    memset(lastSerialBuffer, 0, sizeof(lastSerialBuffer));
    lastSerialCount = 0;
    initSharedJsonIOBuffer(testJsonBuf, TEST_JSON_BUFFER_SIZE);
}

void tearDown(void) {
}

void test_ModeManager_LoadMode_ReadsFromStorage(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial));

    loadMode(&manager, 1);

    TEST_ASSERT_EQUAL_UINT8(1, lastReadModeIndex);
    TEST_ASSERT_EQUAL_UINT8(1, manager.currentModeIndex);
}

void test_ModeManager_IsFakeOff_ReturnsTrueForFakeOffIndex(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial));

    manager.currentModeIndex = FAKE_OFF_MODE_INDEX;
    TEST_ASSERT_TRUE(isFakeOff(&manager));

    manager.currentModeIndex = 0;
    TEST_ASSERT_FALSE(isFakeOff(&manager));
}

void test_ModeManager_LoadMode_FakeOff_DoesNotReadFlash(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial));

    fakeOffMode(&manager);

    TEST_ASSERT_EQUAL_UINT8(255, lastReadModeIndex);  // Should not have changed from init value
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, manager.currentModeIndex);
}

void test_ModeManager_LoadMode_FakeOff_ShouldKeepLedTimersRunning(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial));

    fakeOffMode(&manager);

    TEST_ASSERT_EQUAL_UINT8(255, lastReadModeIndex);  // Should not have changed from init value
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, manager.currentModeIndex);
}

void test_ModeManager_LoadMode_EnablesAccel_IfModeHasAccel(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial));

    loadMode(&manager, 2);  // Mode 2 has accel

    TEST_ASSERT_TRUE(accelEnabled);
}

void test_ModeManager_LoadMode_DisablesAccel_IfModeHasNoAccel(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial));

    loadMode(&manager, 3);  // Mode 3 has no accel

    TEST_ASSERT_TRUE(accelDisabled);
}

void test_UpdateMode_FrontLed_FollowsSimplePattern(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup a simple pattern: High at 0ms, Low at 500ms
    manager.currentMode.hasFront = true;
    manager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.front.pattern.data.simple.duration = 1000;
    manager.currentMode.front.pattern.data.simple.changeAtCount = 2;
    manager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = high;
    manager.currentMode.front.pattern.data.simple.changeAt[1].ms = 500;
    manager.currentMode.front.pattern.data.simple.changeAt[1].output.type = BULB;
    manager.currentMode.front.pattern.data.simple.changeAt[1].output.data.bulb = low;

    // Reset state
    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Test at 100ms (should be High)
    modeTask(&manager, 100, true, 50);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    // Test at 600ms (should be Low)
    modeTask(&manager, 600, true, 50);
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_UpdateMode_FrontRgb_EnablesFrontTimerAndWritesRgb(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    manager.currentMode.hasFront = true;
    manager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.front.pattern.data.simple.duration = 1000;
    manager.currentMode.front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.rgb.r = 10;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.rgb.g = 20;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.rgb.b = 30;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    ModeOutputs outputs = modeTask(&manager, 100, true, 50);

    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
    TEST_ASSERT_EQUAL_UINT8(10, lastFrontRgbR);
    TEST_ASSERT_EQUAL_UINT8(20, lastFrontRgbG);
    TEST_ASSERT_EQUAL_UINT8(30, lastFrontRgbB);
    TEST_ASSERT_TRUE(outputs.frontValid);
    TEST_ASSERT_EQUAL_UINT8(RGB, outputs.frontType);
}

void test_ModeTask_ReturnsCaseRgbActive(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    manager.currentMode.hasCaseComp = true;
    manager.currentMode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.r = 1;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.g = 2;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.b = 3;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    ModeOutputs outputs = modeTask(&manager, 10, true, 50);

    TEST_ASSERT_TRUE(outputs.caseValid);
}

void test_ModeTask_NoFrontComponent_ClearsBulbAndFrontOutput(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    manager.currentMode.hasFront = false;
    lastWrittenBulbState = 1;

    ModeOutputs outputs = modeTask(&manager, 10, true, 50);

    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
    TEST_ASSERT_FALSE(outputs.frontValid);
}

void test_FrontPattern_ContinuesDuringTriggerOverride(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    manager.currentMode.hasFront = true;
    manager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.front.pattern.data.simple.duration = 2000;
    manager.currentMode.front.pattern.data.simple.changeAtCount = 2;
    manager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = high;
    manager.currentMode.front.pattern.data.simple.changeAt[1].ms = 1000;
    manager.currentMode.front.pattern.data.simple.changeAt[1].output.type = BULB;
    manager.currentMode.front.pattern.data.simple.changeAt[1].output.data.bulb = low;

    manager.currentMode.hasAccel = true;
    manager.currentMode.accel.triggersCount = 1;
    manager.currentMode.accel.triggers[0].threshold = 10;
    manager.currentMode.accel.triggers[0].hasFront = true;
    manager.currentMode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.duration = 100;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.data.bulb =
        high;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    mockAccelMagnitude = 0;
    modeTask(&manager, 100, true, 50);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    mockAccelMagnitude = 20;
    modeTask(&manager, 600, true, 50);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    modeTask(&manager, 1200, true, 50);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    mockAccelMagnitude = 0;
    modeTask(&manager, 1300, true, 50);
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_UpdateMode_CaseLed_FollowsSimplePattern(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup a simple pattern for Case LED
    manager.currentMode.hasCaseComp = true;
    manager.currentMode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.r = 255;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.b = 128;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Test at 100ms
    modeTask(&manager, 100, true, 50);

    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(128, lastRgbB);
}

void test_UpdateMode_CaseLed_Off_WhenNoPattern(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    manager.currentMode.hasCaseComp = false;

    // Set last RGB to something else
    lastRgbR = 10;
    lastRgbG = 10;
    lastRgbB = 10;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    modeTask(&manager, 100, true, 50);

    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_UpdateMode_CaseLed_NotUpdated_WhenButtonEvaluating(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup pattern
    manager.currentMode.hasCaseComp = true;
    manager.currentMode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.r = 255;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.g = 255;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.b = 255;

    // Set last RGB to something else
    lastRgbR = 50;
    lastRgbG = 50;
    lastRgbB = 50;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Pass false for canUpdateCaseLed
    modeTask(&manager, 100, false, 50);

    // Should NOT have updated to 255, 255, 255
    TEST_ASSERT_EQUAL_UINT8(50, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(50, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(50, lastRgbB);
}

void test_UpdateMode_CaseLed_FollowsSimplePatternMultipleChanges(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup a simple pattern for Case LED
    manager.currentMode.hasCaseComp = true;
    manager.currentMode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.caseComp.pattern.data.simple.duration = 2000;
    manager.currentMode.caseComp.pattern.data.simple.changeAtCount = 3;

    // 0ms: Red
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.r = 255;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.b = 0;

    // 500ms: Green
    manager.currentMode.caseComp.pattern.data.simple.changeAt[1].ms = 500;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[1].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[1].output.data.rgb.r = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[1].output.data.rgb.g = 255;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[1].output.data.rgb.b = 0;

    // 1000ms: Blue
    manager.currentMode.caseComp.pattern.data.simple.changeAt[2].ms = 1000;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[2].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[2].output.data.rgb.r = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[2].output.data.rgb.g = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[2].output.data.rgb.b = 255;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Test at 100ms (Should be Red)
    modeTask(&manager, 100, true, 50);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Test at 600ms (Should be Green)
    modeTask(&manager, 600, true, 50);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Test at 1500ms (Should be Blue)
    modeTask(&manager, 1500, true, 50);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbB);

    // Test at 2100ms (Should be Red - loop back to 100ms)
    modeTask(&manager, 2100, true, 50);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_UpdateMode_AccelTrigger_OverridesPatterns_WhenThresholdMet(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup Default Mode: Front OFF, Case OFF
    manager.currentMode.hasFront = true;
    manager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.front.pattern.data.simple.duration = 1000;
    manager.currentMode.front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = low;

    manager.currentMode.hasCaseComp = true;
    manager.currentMode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.r = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.b = 0;

    // Setup Accel Trigger: Front ON, Case RED
    manager.currentMode.hasAccel = true;
    manager.currentMode.accel.triggersCount = 1;
    manager.currentMode.accel.triggers[0].threshold = 10;

    manager.currentMode.accel.triggers[0].hasFront = true;
    manager.currentMode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.duration = 1000;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.data.bulb =
        high;

    manager.currentMode.accel.triggers[0].hasCaseComp = true;
    manager.currentMode.accel.triggers[0].caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.changeAt[0].output.type =
        RGB;
    manager.currentMode.accel.triggers[0]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.r = 255;
    manager.currentMode.accel.triggers[0]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.g = 0;
    manager.currentMode.accel.triggers[0]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.b = 0;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Trigger the accel
    mockAccelMagnitude = 20;

    modeTask(&manager, 100, true, 50);

    // Should be High and Red
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_UpdateMode_AccelTrigger_DoesNotOverride_WhenThresholdNotMet(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup Default Mode: Front OFF
    manager.currentMode.hasFront = true;
    manager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.front.pattern.data.simple.duration = 1000;
    manager.currentMode.front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = low;

    // Setup Accel Trigger: Front ON
    manager.currentMode.hasAccel = true;
    manager.currentMode.accel.triggersCount = 1;
    manager.currentMode.accel.triggers[0].hasFront = true;
    manager.currentMode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.duration = 1000;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.data.bulb =
        high;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Do NOT trigger the accel
    mockAccelMagnitude = 0;

    modeTask(&manager, 100, true, 50);

    // Should be Low (Default)
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_UpdateMode_AccelTrigger_PartialOverride(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup Default Mode: Front OFF, Case BLUE
    manager.currentMode.hasFront = true;
    manager.currentMode.front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.front.pattern.data.simple.duration = 1000;
    manager.currentMode.front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.front.pattern.data.simple.changeAt[0].output.data.bulb = low;

    manager.currentMode.hasCaseComp = true;
    manager.currentMode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.r = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.b = 255;

    // Setup Accel Trigger: Front ON, NO Case override
    manager.currentMode.hasAccel = true;
    manager.currentMode.accel.triggersCount = 1;

    manager.currentMode.accel.triggers[0].hasFront = true;
    manager.currentMode.accel.triggers[0].front.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.duration = 1000;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.type = BULB;
    manager.currentMode.accel.triggers[0].front.pattern.data.simple.changeAt[0].output.data.bulb =
        high;

    manager.currentMode.accel.triggers[0].hasCaseComp = false;  // No override

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Trigger the accel
    mockAccelMagnitude = 20;

    modeTask(&manager, 100, true, 50);

    // Should be High (Triggered) and Blue (Default)
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbB);
}
void test_UpdateMode_AccelTrigger_UsesHighestMatchingTrigger_AssumingAscendingOrder(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial);

    // Setup Default Mode: Case OFF
    manager.currentMode.hasCaseComp = true;
    manager.currentMode.caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.type = RGB;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.r = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.g = 0;
    manager.currentMode.caseComp.pattern.data.simple.changeAt[0].output.data.rgb.b = 0;

    manager.currentMode.hasAccel = true;
    manager.currentMode.accel.triggersCount = 2;

    // Trigger 0: Low Threshold (10) -> BLUE
    manager.currentMode.accel.triggers[0].threshold = 10;
    manager.currentMode.accel.triggers[0].hasCaseComp = true;
    manager.currentMode.accel.triggers[0].caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.accel.triggers[0].caseComp.pattern.data.simple.changeAt[0].output.type =
        RGB;
    manager.currentMode.accel.triggers[0]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.r = 0;
    manager.currentMode.accel.triggers[0]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.g = 0;
    manager.currentMode.accel.triggers[0]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.b = 255;

    // Trigger 1: High Threshold (20) -> RED
    manager.currentMode.accel.triggers[1].threshold = 20;
    manager.currentMode.accel.triggers[1].hasCaseComp = true;
    manager.currentMode.accel.triggers[1].caseComp.pattern.type = PATTERN_TYPE_SIMPLE;
    manager.currentMode.accel.triggers[1].caseComp.pattern.data.simple.duration = 1000;
    manager.currentMode.accel.triggers[1].caseComp.pattern.data.simple.changeAtCount = 1;
    manager.currentMode.accel.triggers[1].caseComp.pattern.data.simple.changeAt[0].ms = 0;
    manager.currentMode.accel.triggers[1].caseComp.pattern.data.simple.changeAt[0].output.type =
        RGB;
    manager.currentMode.accel.triggers[1]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.r = 255;
    manager.currentMode.accel.triggers[1]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.g = 0;
    manager.currentMode.accel.triggers[1]
        .caseComp.pattern.data.simple.changeAt[0]
        .output.data.rgb.b = 0;

    modeStateInitialize(&manager.modeState, &manager.currentMode, 0, NULL);
    manager.shouldResetState = false;

    // Case A: Accel = 5 (Below both) -> Default (OFF)
    mockAccelMagnitude = 5;
    modeTask(&manager, 100, true, 50);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Case B: Accel = 15 (Above Trigger 0, Below Trigger 1) -> Trigger 0 (BLUE)
    mockAccelMagnitude = 15;
    modeTask(&manager, 200, true, 50);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbB);

    // Case C: Accel = 25 (Above both) -> Trigger 1 (RED)
    mockAccelMagnitude = 25;
    modeTask(&manager, 300, true, 50);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_ModeManager_LogsEquationCompileError(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        &mockFrontLed,
        mock_readSavedMode,
        mock_writeBulbLedPin,
        mock_writeToSerial));

    manager.currentMode.hasFront = true;
    manager.currentMode.front.pattern.type = PATTERN_TYPE_EQUATION;
    EquationPattern *pattern = &manager.currentMode.front.pattern.data.equation;
    pattern->red.sectionsCount = 1;
    strcpy(pattern->red.sections[0].equation, "bad +");
    pattern->red.sections[0].duration = 1000;

    modeTask(&manager, 0, true, 50);

    TEST_ASSERT_TRUE(writeToSerialCalled);
    TEST_ASSERT_NOT_EQUAL_UINT32(0, lastSerialCount);
    TEST_ASSERT_NOT_NULL(strstr(lastSerialBuffer, "front"));
    TEST_ASSERT_NOT_NULL(strstr(lastSerialBuffer, "red"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_FrontPattern_ContinuesDuringTriggerOverride);
    RUN_TEST(test_ModeManager_IsFakeOff_ReturnsTrueForFakeOffIndex);
    RUN_TEST(test_ModeManager_LoadMode_DisablesAccel_IfModeHasNoAccel);
    RUN_TEST(test_ModeManager_LoadMode_EnablesAccel_IfModeHasAccel);
    RUN_TEST(test_ModeManager_LoadMode_FakeOff_DoesNotReadFlash);
    RUN_TEST(test_ModeManager_LoadMode_FakeOff_ShouldKeepLedTimersRunning);
    RUN_TEST(test_ModeManager_LoadMode_ReadsFromStorage);
    RUN_TEST(test_ModeManager_LogsEquationCompileError);
    RUN_TEST(test_ModeTask_NoFrontComponent_ClearsBulbAndFrontOutput);
    RUN_TEST(test_ModeTask_ReturnsCaseRgbActive);
    RUN_TEST(test_UpdateMode_AccelTrigger_DoesNotOverride_WhenThresholdNotMet);
    RUN_TEST(test_UpdateMode_AccelTrigger_OverridesPatterns_WhenThresholdMet);
    RUN_TEST(test_UpdateMode_AccelTrigger_PartialOverride);
    RUN_TEST(test_UpdateMode_AccelTrigger_UsesHighestMatchingTrigger_AssumingAscendingOrder);
    RUN_TEST(test_UpdateMode_CaseLed_FollowsSimplePattern);
    RUN_TEST(test_UpdateMode_CaseLed_FollowsSimplePatternMultipleChanges);
    RUN_TEST(test_UpdateMode_CaseLed_NotUpdated_WhenButtonEvaluating);
    RUN_TEST(test_UpdateMode_CaseLed_Off_WhenNoPattern);
    RUN_TEST(test_UpdateMode_FrontLed_FollowsSimplePattern);
    RUN_TEST(test_UpdateMode_FrontRgb_EnablesFrontTimerAndWritesRgb);
    return UNITY_END();
}
