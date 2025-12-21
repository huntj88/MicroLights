#include "unity.h"
#include <string.h>
#include <stdbool.h>

#include "mode_manager.h"
#include "storage.h"
#include "json/command_parser.h"
#include "device/mc3479.h"

// Mock Data
CliInput cliInput;
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

void readBulbModeFromFlash(uint8_t mode, char *buffer, uint32_t length) {
    lastReadModeIndex = mode;
    // Simulate reading valid JSON
    strcpy(buffer, "{\"command\":\"writeMode\",\"index\":0,\"mode\":{\"name\":\"test\"}}");
}

void parseJson(uint8_t buf[], uint32_t count, CliInput *input) {
    // Simulate parsing
    if (strstr((char*)buf, "fakeOff") != NULL) {
        input->parsedType = parseWriteMode;
        input->modeIndex = FAKE_OFF_MODE_INDEX;
    } else if (strstr((char*)buf, "writeMode") != NULL) {
        input->parsedType = parseWriteMode;
        input->modeIndex = 0;
    } else {
        input->parsedType = parseError;
    }
}

// Include source
#include "../Core/Src/mode_manager.c"

void setUp(void) {
    memset(&cliInput, 0, sizeof(CliInput));
    memset(&mockAccel, 0, sizeof(MC3479));
    ledTimersStarted = false;
    ledTimersStopped = false;
    accelEnabled = false;
    accelDisabled = false;
    lastReadModeIndex = 255;
    memset(lastReadBuffer, 0, sizeof(lastReadBuffer));
}

void tearDown(void) {}

void test_ModeManager_LoadMode_ReadsFromStorage(void) {
    ModeManager manager;
    modeManagerInit(&manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers);
    
    loadMode(&manager, 1);
    
    TEST_ASSERT_EQUAL_UINT8(1, lastReadModeIndex);
    TEST_ASSERT_EQUAL_UINT8(1, manager.currentModeIndex);
    TEST_ASSERT_TRUE(ledTimersStarted);
}

void test_ModeManager_IsFakeOff_ReturnsTrueForFakeOffIndex(void) {
    ModeManager manager;
    modeManagerInit(&manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers);
    
    manager.currentModeIndex = FAKE_OFF_MODE_INDEX;
    TEST_ASSERT_TRUE(isFakeOff(&manager));
    
    manager.currentModeIndex = 0;
    TEST_ASSERT_FALSE(isFakeOff(&manager));
}

void test_ModeManager_LoadMode_FakeOff_DoesNotReadFlash(void) {
    ModeManager manager;
    modeManagerInit(&manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers);
    
    loadMode(&manager, FAKE_OFF_MODE_INDEX);
    
    TEST_ASSERT_EQUAL_UINT8(255, lastReadModeIndex); // Should not have changed from init value
    TEST_ASSERT_EQUAL_UINT8(FAKE_OFF_MODE_INDEX, manager.currentModeIndex);
    TEST_ASSERT_TRUE(ledTimersStopped);
}

void test_ModeManager_LoadMode_EnablesAccel_IfModeHasAccel(void) {
    ModeManager manager;
    modeManagerInit(&manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers);
    
    // Setup cliInput to simulate a mode with accel
    cliInput.mode.has_accel = true;
    cliInput.mode.accel.triggers_count = 1;
    
    // We need to override the parseJson mock to populate cliInput correctly for this test
    // Or just call setMode directly since loadMode calls setMode
    setMode(&manager, &cliInput.mode, 0);
    
    TEST_ASSERT_TRUE(accelEnabled);
}

void test_ModeManager_LoadMode_DisablesAccel_IfModeHasNoAccel(void) {
    ModeManager manager;
    modeManagerInit(&manager, &mockAccel, mock_startLedTimers, mock_stopLedTimers);
    
    cliInput.mode.has_accel = false;
    
    setMode(&manager, &cliInput.mode, 0);
    
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
