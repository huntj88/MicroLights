#include <stdbool.h>
#include <string.h>
#include "unity.h"

#include "microlight/device/button.h"
#include "microlight/device/rgb_led.h"

// Mock Data
static Button button;
static uint8_t mockButtonPinState = 1;  // 1 = released, 0 = pressed

// Mock Functions
uint8_t mock_readButtonPin() {
    return mockButtonPinState;
}

// Include source
#include "../../../Core/Src/microlight/device/button.c"

void setUp(void) {
    memset(&button, 0, sizeof(Button));
    mockButtonPinState = 1;

    buttonInit(&button, mock_readButtonPin);
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
    enum ButtonResult result = buttonInputTask(&button, 100, interrupt);
    TEST_ASSERT_EQUAL(ignore, result);
    TEST_ASSERT_TRUE(isEvaluatingButtonPress(&button));

    // Advance time, button still pressed
    buttonInputTask(&button, 200, false);  // 100ms elapsed

    // Release button
    mockButtonPinState = 1;
    result = buttonInputTask(&button, 300, false);  // 200ms elapsed total

    TEST_ASSERT_EQUAL(clicked, result);
    TEST_ASSERT_FALSE(isEvaluatingButtonPress(&button));
}

void test_ButtonInputTask_ReturnsShutdown_AfterLongPress(void) {
    bool interrupt = true;
    mockButtonPinState = 0;
    buttonInputTask(&button, 100, interrupt);

    // Advance past the shutdown hold threshold (>500ms elapsed)
    enum ButtonResult result = buttonInputTask(&button, 650, false);
    TEST_ASSERT_EQUAL(indicateShutdown, result);

    // Release
    mockButtonPinState = 1;
    result = buttonInputTask(&button, 800, false);

    TEST_ASSERT_EQUAL(shutdown, result);
}

void test_ButtonInputTask_ReturnsLock_AfterVeryLongPress(void) {
    bool interrupt = true;
    mockButtonPinState = 0;
    buttonInputTask(&button, 100, interrupt);

    // Past the shutdown hold threshold (>500ms elapsed)
    enum ButtonResult result = buttonInputTask(&button, 650, false);
    TEST_ASSERT_EQUAL(indicateShutdown, result);

    // Past the lock hold threshold (>1500ms elapsed)
    result = buttonInputTask(&button, 1650, false);
    TEST_ASSERT_EQUAL(indicateLockOrHardwareReset, result);

    // Release
    mockButtonPinState = 1;
    result = buttonInputTask(&button, 1800, false);

    TEST_ASSERT_EQUAL(lockOrHardwareReset, result);
}

void test_ButtonInputTask_IndicatesStatus_WhileHeld(void) {
    bool interrupt = true;
    mockButtonPinState = 0;
    enum ButtonResult result = buttonInputTask(&button, 100, interrupt);

    // Below the shutdown threshold, nothing to indicate yet
    TEST_ASSERT_EQUAL(ignore, result);

    // Shutdown feedback window (>500ms elapsed)
    result = buttonInputTask(&button, 650, false);
    TEST_ASSERT_EQUAL(indicateShutdown, result);

    // Lock feedback window (>1500ms elapsed)
    result = buttonInputTask(&button, 1650, false);
    TEST_ASSERT_EQUAL(indicateLockOrHardwareReset, result);
}

void test_ButtonInputTask_IgnoresReleasedInterruptBounce(void) {
    enum ButtonResult result = buttonInputTask(&button, 100, true);

    TEST_ASSERT_EQUAL(ignore, result);
    TEST_ASSERT_FALSE(isEvaluatingButtonPress(&button));

    result = buttonInputTask(&button, 200, false);

    TEST_ASSERT_EQUAL(ignore, result);
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
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_ButtonInputTask_CancelsPressReleasedBeforeDebounce);
    RUN_TEST(test_ButtonInputTask_IgnoresReleasedInterruptBounce);
    RUN_TEST(test_ButtonInputTask_IndicatesStatus_WhileHeld);
    RUN_TEST(test_ButtonInputTask_ReturnsClicked_AfterShortPress);
    RUN_TEST(test_ButtonInputTask_ReturnsIgnore_Idle);
    RUN_TEST(test_ButtonInputTask_ReturnsLock_AfterVeryLongPress);
    RUN_TEST(test_ButtonInputTask_ReturnsShutdown_AfterLongPress);
    return UNITY_END();
}
