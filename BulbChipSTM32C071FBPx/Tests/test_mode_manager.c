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
static bool ledTimersStarted = false;
static bool ledTimersStopped = false;
static bool accelEnabled = false;
static bool accelDisabled = false;
static uint8_t lastReadModeIndex = 255;
static char lastReadBuffer[1024];

// Mock Functions
void mock_startLedTimers() {
    ledTimersStarted = true;
}

void mock_stopLedTimers() {
    ledTimersStopped = true;
}

void mc3479Enable(MC3479 *dev) {
    accelEnabled = true;
}

void mc3479Disable(MC3479 *dev) {
    accelDisabled = true;
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

void setUp(void) {
    memset(&mockAccel, 0, sizeof(MC3479));
    ledTimersStarted = false;
    ledTimersStopped = false;
    accelEnabled = false;
    accelDisabled = false;
    lastReadModeIndex = 255;
    memset(lastReadBuffer, 0, sizeof(lastReadBuffer));
}

void tearDown(void) {
}

void test_ModeManager_LoadMode_ReadsFromStorage(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers, mock_readBulbModeFromFlash));

    loadMode(&manager, 1);

    TEST_ASSERT_EQUAL_UINT8(1, lastReadModeIndex);
    TEST_ASSERT_EQUAL_UINT8(1, manager.currentModeIndex);
    TEST_ASSERT_TRUE(ledTimersStarted);
}

void test_ModeManager_IsFakeOff_ReturnsTrueForFakeOffIndex(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers, mock_readBulbModeFromFlash));

    manager.currentModeIndex = FAKE_OFF_MODE_INDEX;
    TEST_ASSERT_TRUE(isFakeOff(&manager));

    manager.currentModeIndex = 0;
    TEST_ASSERT_FALSE(isFakeOff(&manager));
}

void test_ModeManager_LoadMode_FakeOff_DoesNotReadFlash(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers, mock_readBulbModeFromFlash));

    loadMode(&manager, FAKE_OFF_MODE_INDEX);

    TEST_ASSERT_EQUAL_UINT8(255, lastReadModeIndex);  // Should not have changed from init value
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, manager.currentModeIndex);
    TEST_ASSERT_TRUE(ledTimersStopped);
}

void test_ModeManager_LoadMode_EnablesAccel_IfModeHasAccel(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers, mock_readBulbModeFromFlash));

    loadMode(&manager, 2);  // Mode 2 has accel

    TEST_ASSERT_TRUE(accelEnabled);
}

void test_ModeManager_LoadMode_DisablesAccel_IfModeHasNoAccel(void) {
    ModeManager manager;
    TEST_ASSERT_TRUE(modeManagerInit(
        &manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers, mock_readBulbModeFromFlash));

    loadMode(&manager, 3);  // Mode 3 has no accel

    TEST_ASSERT_TRUE(accelDisabled);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ModeManager_LoadMode_ReadsFromStorage);
    RUN_TEST(test_ModeManager_IsFakeOff_ReturnsTrueForFakeOffIndex);
    RUN_TEST(test_ModeManager_LoadMode_FakeOff_DoesNotReadFlash);
    RUN_TEST(test_ModeManager_LoadMode_EnablesAccel_IfModeHasAccel);
    RUN_TEST(test_ModeManager_LoadMode_DisablesAccel_IfModeHasNoAccel);
    return UNITY_END();
}
