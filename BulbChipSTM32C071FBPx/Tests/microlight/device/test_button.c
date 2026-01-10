#include <stdbool.h>
#include <string.h>
#include "unity.h"

#include "microlight/device/button.h"
#include "microlight/device/rgb_led.h"

// Mock Data
static Button button;
static RGBLed mockCaseLed;
static uint8_t mockButtonPinState = 1;  // 1 = released, 0 = pressed
static bool timerStarted = false;

// Mock Functions
uint8_t mock_readButtonPin() {
    return mockButtonPinState;
}

void mock_enableTimers(bool enable) {
    timerStarted = enable;
}

// RGB Mocks
bool rgbNoColorCalled = false;
bool rgbLockedCalled = false;
bool rgbShutdownCalled = false;
bool rgbSuccessCalled = false;

void rgbShowNoColor(RGBLed *led) {
    rgbNoColorCalled = true;
}
void rgbShowLocked(RGBLed *led) {
    rgbLockedCalled = true;
}
void rgbShowShutdown(RGBLed *led) {
    rgbShutdownCalled = true;
}
void rgbShowSuccess(RGBLed *led) {
    rgbSuccessCalled = true;
}
void rgbShowUserColor(RGBLed *led, uint8_t r, uint8_t g, uint8_t b) {
}

// Include source
#include "../../../Core/Src/microlight/device/button.c"

void setUp(void) {
    memset(&button, 0, sizeof(Button));
    memset(&mockCaseLed, 0, sizeof(RGBLed));
    mockButtonPinState = 1;
    timerStarted = false;
    rgbNoColorCalled = false;
    rgbLockedCalled = false;
    rgbShutdownCalled = false;
    rgbSuccessCalled = false;

    buttonInit(&button, mock_readButtonPin, mock_enableTimers, &mockCaseLed);
}

void tearDown(void) {
}

void test_ButtonInputTask_ReturnsIgnore_Idle(void) {
    enum ButtonResult result = buttonInputTask(&button, 100, false);
    TEST_ASSERT_EQUAL(ignore, result);
}

void test_ButtonInputTask_ReturnsClicked_AfterShortPress(void) {
    // Determine if interrupt
    bool interrupt = true;

    // First call, starts evaluation
    mockButtonPinState = 0;  // Pressed
    buttonInputTask(&button, 100, interrupt);
    TEST_ASSERT_TRUE(timerStarted);
    TEST_ASSERT_TRUE(rgbNoColorCalled);
    TEST_ASSERT_TRUE(isEvaluatingButtonPress(&button));

    // Advance time, button still pressed
    buttonInputTask(&button, 200, false);  // 100ms elapsed

    // Release button
    mockButtonPinState = 1;
    enum ButtonResult result = buttonInputTask(&button, 300, false);  // 200ms elapsed total

    TEST_ASSERT_EQUAL(clicked, result);
    TEST_ASSERT_TRUE(rgbSuccessCalled);
    TEST_ASSERT_FALSE(isEvaluatingButtonPress(&button));
}

void test_ButtonInputTask_ReturnsShutdown_AfterLongPress(void) {
    bool interrupt = true;
    mockButtonPinState = 0;
    buttonInputTask(&button, 100, interrupt);

    // Advance time past 1000ms
    buttonInputTask(&button, 1150, false);
    TEST_ASSERT_TRUE(rgbShutdownCalled);

    // Release
    mockButtonPinState = 1;
    enum ButtonResult result = buttonInputTask(&button, 1300, false);

    TEST_ASSERT_EQUAL(shutdown, result);
}

void test_ButtonInputTask_UpdatesCaseLed_DuringPress(void) {
    bool interrupt = true;
    mockButtonPinState = 0;
    buttonInputTask(&button, 100, interrupt);

    TEST_ASSERT_TRUE(rgbNoColorCalled);

    // Check shutdown feedback
    buttonInputTask(&button, 1150, false);
    TEST_ASSERT_TRUE(rgbShutdownCalled);

    // Check lock feedback
    buttonInputTask(&button, 2150, false);
    TEST_ASSERT_TRUE(rgbLockedCalled);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ButtonInputTask_ReturnsClicked_AfterShortPress);
    RUN_TEST(test_ButtonInputTask_ReturnsIgnore_Idle);
    RUN_TEST(test_ButtonInputTask_ReturnsShutdown_AfterLongPress);
    RUN_TEST(test_ButtonInputTask_UpdatesCaseLed_DuringPress);
    return UNITY_END();
}
