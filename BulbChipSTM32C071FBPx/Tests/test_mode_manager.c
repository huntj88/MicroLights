#include <stdbool.h>
#include <string.h>
#include "unity.h"

#include "device/mc3479.h"
#include "json/command_parser.h"
#include "json/mode_parser.h"
#include "mode_manager.h"
#include "model/cli_model.h"
#include "storage.h"

static MC3479 mockAccel;
static RGBLed mockCaseLed;
static bool ledTimersStarted = false;
static bool accelEnabled = false;
static bool accelDisabled = false;
static uint8_t lastReadModeIndex = 255;
static char lastReadBuffer[PAGE_SECTOR];
static uint8_t lastWrittenBulbState = 255;
static uint8_t lastRgbR, lastRgbG, lastRgbB;
static uint8_t mockAccelMagnitude = 0;

// Mock Functions
void mock_enableTimers(bool enable) {
    ledTimersStarted = enable;
}

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
    lastRgbR = r;
    lastRgbG = g;
    lastRgbB = b;
}

bool isOverThreshold(MC3479 *dev, uint8_t threshold) {
    return mockAccelMagnitude > threshold;
}

void mock_readBulbModeFromFlash(uint8_t mode, char *buffer, uint32_t length) {
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
#include "../Core/Src/mode_manager.c"
#include "../Core/Src/model/mode_state.c"

void setUp(void) {
    memset(&mockAccel, 0, sizeof(MC3479));
    memset(&mockCaseLed, 0, sizeof(RGBLed));
    ledTimersStarted = false;
    accelEnabled = false;
    accelDisabled = false;
    lastReadModeIndex = 255;
    memset(lastReadBuffer, 0, sizeof(lastReadBuffer));
    lastWrittenBulbState = 255;
    lastRgbR = 0;
    lastRgbG = 0;
    lastRgbB = 0;
    mockAccelMagnitude = 0;
}

void tearDown(void) {
}

void test_ModeManager_LoadMode_ReadsFromStorage(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin));

    loadMode(&manager, 1);

    TEST_ASSERT_EQUAL_UINT8(1, lastReadModeIndex);
    TEST_ASSERT_EQUAL_UINT8(1, manager.currentModeIndex);
    TEST_ASSERT_TRUE(ledTimersStarted);
}

void test_ModeManager_IsFakeOff_ReturnsTrueForFakeOffIndex(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin));

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
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin));

    fakeOffMode(&manager, false);

    TEST_ASSERT_EQUAL_UINT8(255, lastReadModeIndex);  // Should not have changed from init value
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, manager.currentModeIndex);
    TEST_ASSERT_FALSE(ledTimersStarted);
}

void test_ModeManager_LoadMode_FakeOff_ShouldKeepLedTimersRunning(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin));

    fakeOffMode(&manager, true);

    TEST_ASSERT_EQUAL_UINT8(255, lastReadModeIndex);  // Should not have changed from init value
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, manager.currentModeIndex);
    TEST_ASSERT_TRUE(ledTimersStarted);
}

void test_ModeManager_LoadMode_EnablesAccel_IfModeHasAccel(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin));

    loadMode(&manager, 2);  // Mode 2 has accel

    TEST_ASSERT_TRUE(accelEnabled);
}

void test_ModeManager_LoadMode_DisablesAccel_IfModeHasNoAccel(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin));

    loadMode(&manager, 3);  // Mode 3 has no accel

    TEST_ASSERT_TRUE(accelDisabled);
}

void test_UpdateMode_FrontLed_FollowsSimplePattern(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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
    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Test at 100ms (should be High)
    modeTask(&manager, 100, true);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    // Test at 600ms (should be Low)
    modeTask(&manager, 600, true);
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_FrontPattern_ContinuesDuringTriggerOverride(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    mockAccelMagnitude = 0;
    modeTask(&manager, 100, true);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    mockAccelMagnitude = 20;
    modeTask(&manager, 600, true);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    modeTask(&manager, 1200, true);
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);

    mockAccelMagnitude = 0;
    modeTask(&manager, 1300, true);
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_UpdateMode_CaseLed_FollowsSimplePattern(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Test at 100ms
    modeTask(&manager, 100, true);

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
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

    manager.currentMode.hasCaseComp = false;

    // Set last RGB to something else
    lastRgbR = 10;
    lastRgbG = 10;
    lastRgbB = 10;

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    modeTask(&manager, 100, true);

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
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Pass false for canUpdateCaseLed
    modeTask(&manager, 100, false);

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
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Test at 100ms (Should be Red)
    modeTask(&manager, 100, true);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Test at 600ms (Should be Green)
    modeTask(&manager, 600, true);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Test at 1500ms (Should be Blue)
    modeTask(&manager, 1500, true);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbB);

    // Test at 2100ms (Should be Red - loop back to 100ms)
    modeTask(&manager, 2100, true);
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
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Trigger the accel
    mockAccelMagnitude = 20;

    modeTask(&manager, 100, true);

    // Should be High and Red
    TEST_ASSERT_EQUAL_UINT8(1, lastWrittenBulbState);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbG);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
}

void test_UpdateMode_AccelTrigger_DoesNotOverride_WhenThresholdNotMet(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Do NOT trigger the accel
    mockAccelMagnitude = 0;

    modeTask(&manager, 100, true);

    // Should be Low (Default)
    TEST_ASSERT_EQUAL_UINT8(0, lastWrittenBulbState);
}

void test_UpdateMode_AccelTrigger_PartialOverride(void) {
    ModeManager manager;
    modeManagerInit(
        &manager,
        &mockAccel,
        &mockCaseLed,
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Trigger the accel
    mockAccelMagnitude = 20;

    modeTask(&manager, 100, true);

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
        mock_enableTimers,
        mock_readBulbModeFromFlash,
        mock_writeBulbLedPin);

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

    modeStateReset(&manager.modeState, &manager.currentMode, 0);
    manager.shouldResetState = false;

    // Case A: Accel = 5 (Below both) -> Default (OFF)
    mockAccelMagnitude = 5;
    modeTask(&manager, 100, true);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);

    // Case B: Accel = 15 (Above Trigger 0, Below Trigger 1) -> Trigger 0 (BLUE)
    mockAccelMagnitude = 15;
    modeTask(&manager, 200, true);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbB);

    // Case C: Accel = 25 (Above both) -> Trigger 1 (RED)
    mockAccelMagnitude = 25;
    modeTask(&manager, 300, true);
    TEST_ASSERT_EQUAL_UINT8(255, lastRgbR);
    TEST_ASSERT_EQUAL_UINT8(0, lastRgbB);
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
    RUN_TEST(test_UpdateMode_AccelTrigger_DoesNotOverride_WhenThresholdNotMet);
    RUN_TEST(test_UpdateMode_AccelTrigger_OverridesPatterns_WhenThresholdMet);
    RUN_TEST(test_UpdateMode_AccelTrigger_PartialOverride);
    RUN_TEST(test_UpdateMode_AccelTrigger_UsesHighestMatchingTrigger_AssumingAscendingOrder);
    RUN_TEST(test_UpdateMode_CaseLed_FollowsSimplePattern);
    RUN_TEST(test_UpdateMode_CaseLed_FollowsSimplePatternMultipleChanges);
    RUN_TEST(test_UpdateMode_CaseLed_NotUpdated_WhenButtonEvaluating);
    RUN_TEST(test_UpdateMode_CaseLed_Off_WhenNoPattern);
    RUN_TEST(test_UpdateMode_FrontLed_FollowsSimplePattern);
    return UNITY_END();
}
