#include <stdbool.h>
#include <string.h>
#include "unity.h"

#include "microlight/device/button.h"
#include "microlight/device/rgb_led.h"

// Mock Data
static Button button;
static RGBLed mockCaseLed;
static RGBLed mockFrontLed;
static uint8_t mockButtonPinState = 1;  // 1 = released, 0 = pressed

// Mock Functions
uint8_t mock_readButtonPin() {
    return mockButtonPinState;
}

// RGB Mocks
bool rgbNoColorCalled = false;
bool rgbLockedCalled = false;
bool rgbShutdownCalled = false;
bool rgbSuccessCalled = false;
bool rgbNoColorCalledFront = false;
bool rgbLockedCalledFront = false;
bool rgbShutdownCalledFront = false;
bool rgbSuccessCalledFront = false;

void rgbShowNoColor(RGBLed *led) {
    if (led == &mockFrontLed) {
        rgbNoColorCalledFront = true;
    } else {
        rgbNoColorCalled = true;
    }
}
void rgbShowLocked(RGBLed *led) {
    if (led == &mockFrontLed) {
        rgbLockedCalledFront = true;
    } else {
        rgbLockedCalled = true;
    }
}
void rgbShowShutdown(RGBLed *led) {
    if (led == &mockFrontLed) {
        rgbShutdownCalledFront = true;
    } else {
        rgbShutdownCalled = true;
    }
}
void rgbShowSuccess(RGBLed *led) {
    if (led == &mockFrontLed) {
        rgbSuccessCalledFront = true;
    } else {
        rgbSuccessCalled = true;
    }
}
void rgbShowUserColor(RGBLed *led, uint8_t r, uint8_t g, uint8_t b) {
}

// Include source
#include "../../../Core/Src/microlight/device/button.c"

void setUp(void) {
    memset(&button, 0, sizeof(Button));
    memset(&mockCaseLed, 0, sizeof(RGBLed));
    memset(&mockFrontLed, 0, sizeof(RGBLed));
    mockButtonPinState = 1;
    rgbNoColorCalled = false;
    rgbLockedCalled = false;
    rgbShutdownCalled = false;
    rgbSuccessCalled = false;
    rgbNoColorCalledFront = false;
    rgbLockedCalledFront = false;
    rgbShutdownCalledFront = false;
    rgbSuccessCalledFront = false;

    buttonInit(&button, mock_readButtonPin, &mockFrontLed, &mockCaseLed);
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
    TEST_ASSERT_TRUE(rgbNoColorCalled);
    TEST_ASSERT_TRUE(isEvaluatingButtonPress(&button));

    // Advance time, button still pressed
    buttonInputTask(&button, 200, false);  // 100ms elapsed

    // Release button
    mockButtonPinState = 1;
    enum ButtonResult result = buttonInputTask(&button, 300, false);  // 200ms elapsed total

    TEST_ASSERT_EQUAL(clicked, result);
    TEST_ASSERT_TRUE(rgbSuccessCalled);
    TEST_ASSERT_FALSE(rgbSuccessCalledFront);
    TEST_ASSERT_FALSE(isEvaluatingButtonPress(&button));
}

void test_ButtonInputTask_ReturnsShutdown_AfterLongPress(void) {
    bool interrupt = true;
    mockButtonPinState = 0;
    buttonInputTask(&button, 100, interrupt);

    // Advance into the shutdown feedback window (500-600ms elapsed)
    buttonInputTask(&button, 650, false);
    TEST_ASSERT_TRUE(rgbShutdownCalled);
    TEST_ASSERT_TRUE(rgbShutdownCalledFront);

    // Release
    mockButtonPinState = 1;
    enum ButtonResult result = buttonInputTask(&button, 800, false);

    TEST_ASSERT_EQUAL(shutdown, result);
}

void test_ButtonInputTask_UpdatesCaseLed_DuringPress(void) {
    bool interrupt = true;
    mockButtonPinState = 0;
    buttonInputTask(&button, 100, interrupt);

    TEST_ASSERT_TRUE(rgbNoColorCalled);
    TEST_ASSERT_TRUE(rgbNoColorCalledFront);

    // Check shutdown feedback (500-600ms elapsed)
    buttonInputTask(&button, 650, false);
    TEST_ASSERT_TRUE(rgbShutdownCalled);
    TEST_ASSERT_TRUE(rgbShutdownCalledFront);

    // Check lock feedback (1500-1600ms elapsed)
    buttonInputTask(&button, 1650, false);
    TEST_ASSERT_TRUE(rgbLockedCalled);
    TEST_ASSERT_TRUE(rgbLockedCalledFront);
}

void test_ButtonInputTask_IgnoresReleasedInterruptBounce(void) {
    enum ButtonResult result = buttonInputTask(&button, 100, true);

    TEST_ASSERT_EQUAL(ignore, result);
    TEST_ASSERT_FALSE(rgbNoColorCalled);
    TEST_ASSERT_FALSE(isEvaluatingButtonPress(&button));

    result = buttonInputTask(&button, 200, false);

    TEST_ASSERT_EQUAL(ignore, result);
    TEST_ASSERT_FALSE(rgbSuccessCalled);
}

void test_ButtonInputTask_CancelsPressReleasedBeforeDebounce(void) {
    mockButtonPinState = 0;
    buttonInputTask(&button, 100, true);

    mockButtonPinState = 1;
    enum ButtonResult result = buttonInputTask(&button, 120, false);

    TEST_ASSERT_EQUAL(ignore, result);
    TEST_ASSERT_FALSE(isEvaluatingButtonPress(&button));

    result = buttonInputTask(&button, 200, false);

    TEST_ASSERT_EQUAL(ignore, result);
    TEST_ASSERT_FALSE(rgbSuccessCalled);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ButtonInputTask_CancelsPressReleasedBeforeDebounce);
    RUN_TEST(test_ButtonInputTask_IgnoresReleasedInterruptBounce);
    RUN_TEST(test_ButtonInputTask_ReturnsClicked_AfterShortPress);
    RUN_TEST(test_ButtonInputTask_ReturnsIgnore_Idle);
    RUN_TEST(test_ButtonInputTask_ReturnsShutdown_AfterLongPress);
    RUN_TEST(test_ButtonInputTask_UpdatesCaseLed_DuringPress);
    return UNITY_END();
}
