#include <stdbool.h>
#include <string.h>

#include "unity.h"

#include "microlight/device/rgb_led.h"

// ── Mock PWM writer ─────────────────────────────────────────────────

static uint16_t capturedRed, capturedGreen, capturedBlue;
static bool writePwmCalled;

static void mock_writePwm(uint16_t red, uint16_t green, uint16_t blue) {
    capturedRed = red;
    capturedGreen = green;
    capturedBlue = blue;
    writePwmCalled = true;
}

// Include source under test
#include "../../../Core/Src/microlight/device/rgb_led.c"

// ── Fixtures ────────────────────────────────────────────────────────

static RGBLed led;

void setUp(void) {
    memset(&led, 0, sizeof(led));
    capturedRed = capturedGreen = capturedBlue = 0;
    writePwmCalled = false;
}

void tearDown(void) {
}

// ── rgbInit ─────────────────────────────────────────────────────────

void test_rgbInit_NullDevice_ReturnsFalse(void) {
    TEST_ASSERT_FALSE(rgbInit(NULL, mock_writePwm, 255));
}

void test_rgbInit_NullCallback_ReturnsFalse(void) {
    TEST_ASSERT_FALSE(rgbInit(&led, NULL, 255));
}

void test_rgbInit_PeriodAbove511_ReturnsFalse(void) {
    TEST_ASSERT_FALSE(rgbInit(&led, mock_writePwm, 512));
}

void test_rgbInit_Period511_Accepted(void) {
    TEST_ASSERT_TRUE(rgbInit(&led, mock_writePwm, 511));
    TEST_ASSERT_EQUAL_UINT16(511, led.period);
}

void test_rgbInit_ValidParams_SetsFieldsCorrectly(void) {
    TEST_ASSERT_TRUE(rgbInit(&led, mock_writePwm, 255));
    TEST_ASSERT_EQUAL_PTR(mock_writePwm, led.writePwm);
    TEST_ASSERT_EQUAL_UINT16(255, led.period);
    TEST_ASSERT_FALSE(led.showingTransientStatus);
    TEST_ASSERT_EQUAL_UINT8(0, led.userRed);
    TEST_ASSERT_EQUAL_UINT8(0, led.userGreen);
    TEST_ASSERT_EQUAL_UINT8(0, led.userBlue);
}

// ── Gamma LUT sanity ────────────────────────────────────────────────

void test_gammaLUT_Endpoints(void) {
    TEST_ASSERT_EQUAL_UINT8(0, gammaLUT[0]);
    TEST_ASSERT_EQUAL_UINT8(255, gammaLUT[255]);
}

void test_gammaLUT_KnownPoints(void) {
    // gammaLUT[15] == 1 (verify neighbouring entry is still 0)
    TEST_ASSERT_EQUAL_UINT8(0, gammaLUT[14]);
    TEST_ASSERT_EQUAL_UINT8(1, gammaLUT[15]);
    // pow(25/255, 2.2)*255 ≈ 1.54 → rounds to 2
    TEST_ASSERT_EQUAL_UINT8(2, gammaLUT[25]);
}

void test_gammaLUT_Monotonic(void) {
    for (int i = 1; i < 256; i++) {
        TEST_ASSERT_TRUE_MESSAGE(
            gammaLUT[i] >= gammaLUT[i - 1], "gammaLUT is not monotonically non-decreasing");
    }
}

// ── colorRangeToDuty boundaries ─────────────────────────────────────

void test_colorRangeToDuty_ZeroInput_ReturnsZero(void) {
    rgbInit(&led, mock_writePwm, 255);
    TEST_ASSERT_EQUAL_UINT16(0, colorRangeToDuty(&led, 0));
}

void test_colorRangeToDuty_MaxInput_ReturnsPeriod255(void) {
    rgbInit(&led, mock_writePwm, 255);
    TEST_ASSERT_EQUAL_UINT16(255, colorRangeToDuty(&led, 255));
}

void test_colorRangeToDuty_MaxInput_ReturnsPeriod511(void) {
    rgbInit(&led, mock_writePwm, 511);
    TEST_ASSERT_EQUAL_UINT16(511, colorRangeToDuty(&led, 255));
}

void test_colorRangeToDuty_MaxInput_ReturnsPeriod100(void) {
    rgbInit(&led, mock_writePwm, 100);
    TEST_ASSERT_EQUAL_UINT16(100, colorRangeToDuty(&led, 255));
}

void test_colorRangeToDuty_MaxInput_ReturnsPeriod1(void) {
    rgbInit(&led, mock_writePwm, 1);
    TEST_ASSERT_EQUAL_UINT16(1, colorRangeToDuty(&led, 255));
}

// ── colorRangeToDuty monotonicity ───────────────────────────────────

void test_colorRangeToDuty_Monotonic_Period255(void) {
    rgbInit(&led, mock_writePwm, 255);
    uint16_t prev = colorRangeToDuty(&led, 0);
    for (int i = 1; i < 256; i++) {
        uint16_t cur = colorRangeToDuty(&led, (uint8_t)i);
        TEST_ASSERT_TRUE_MESSAGE(cur >= prev, "duty not monotonically non-decreasing (period 255)");
        prev = cur;
    }
}

void test_colorRangeToDuty_Monotonic_Period511(void) {
    rgbInit(&led, mock_writePwm, 511);
    uint16_t prev = colorRangeToDuty(&led, 0);
    for (int i = 1; i < 256; i++) {
        uint16_t cur = colorRangeToDuty(&led, (uint8_t)i);
        TEST_ASSERT_TRUE_MESSAGE(cur >= prev, "duty not monotonically non-decreasing (period 511)");
        prev = cur;
    }
}

// ── colorRangeToDuty known computed values ──────────────────────────
// gammaLUT[15] = 1
//   period 255: (1 * 255 * 0x8081) >> 23 = 1
//   period 511: (1 * 511 * 0x8081) >> 23 = 2
// gammaLUT[25] = 2
//   period 511: (2 * 511 * 0x8081) >> 23 = 4

void test_colorRangeToDuty_KnownPoint_Period255_Value15(void) {
    rgbInit(&led, mock_writePwm, 255);
    TEST_ASSERT_EQUAL_UINT16(1, colorRangeToDuty(&led, 15));
}

void test_colorRangeToDuty_KnownPoint_Period511_Value15(void) {
    rgbInit(&led, mock_writePwm, 511);
    TEST_ASSERT_EQUAL_UINT16(2, colorRangeToDuty(&led, 15));
}

void test_colorRangeToDuty_KnownPoint_Period511_Value25(void) {
    rgbInit(&led, mock_writePwm, 511);
    TEST_ASSERT_EQUAL_UINT16(4, colorRangeToDuty(&led, 25));
}

// ── rgbTransientTask ────────────────────────────────────────────────

void test_rgbTransientTask_NullDevice_NoOp(void) {
    rgbTransientTask(NULL, 1000);
    // Should not crash
}

void test_rgbTransientTask_NotTransient_NoChange(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowUserColor(&led, 100, 100, 100);
    writePwmCalled = false;
    rgbTransientTask(&led, 1000);
    TEST_ASSERT_FALSE(writePwmCalled);
}

void test_rgbTransientTask_TransientRevert_After300ms(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowUserColor(&led, 100, 0, 0);
    // Show a transient (success = 50,50,50)
    rgbShowSuccess(&led);
    // Advance time past 300 ms
    writePwmCalled = false;
    rgbTransientTask(&led, 301);
    // Should revert to user color
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 100), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedBlue);
}

void test_rgbTransientTask_TransientHeld_Before300ms(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowUserColor(&led, 100, 0, 0);
    rgbShowSuccess(&led);
    writePwmCalled = false;
    rgbTransientTask(&led, 300);  // exactly 300 ms — not yet expired
    TEST_ASSERT_FALSE(writePwmCalled);
}

// ── rgbShowUserColor while transient ────────────────────────────────

void test_rgbShowUserColor_WhileTransient_StoresButDoesNotDrive(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowSuccess(&led);  // sets transient
    writePwmCalled = false;
    rgbShowUserColor(&led, 200, 100, 50);
    // User color stored but PWM not updated while transient is active
    TEST_ASSERT_FALSE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT8(200, led.userRed);
    TEST_ASSERT_EQUAL_UINT8(100, led.userGreen);
    TEST_ASSERT_EQUAL_UINT8(50, led.userBlue);
}

// ── rgbShow* wrapper colors ─────────────────────────────────────────

void test_rgbShowSuccess_DrivesExpectedColor(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowSuccess(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 50), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 50), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 50), capturedBlue);
    TEST_ASSERT_TRUE(led.showingTransientStatus);
}

void test_rgbShowLocked_DrivesExpectedColor(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowLocked(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 65), capturedBlue);
    TEST_ASSERT_FALSE(led.showingTransientStatus);
}

void test_rgbShowShutdown_DrivesExpectedColor(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowShutdown(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 65), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 65), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 65), capturedBlue);
    TEST_ASSERT_TRUE(led.showingTransientStatus);
}

void test_rgbShowNotCharging_DrivesExpectedColor(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowNotCharging(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 50), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 50), capturedBlue);
    TEST_ASSERT_TRUE(led.showingTransientStatus);
}

void test_rgbShowConstantCurrentCharging_DrivesExpectedColor(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowConstantCurrentCharging(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 25), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedBlue);
    TEST_ASSERT_TRUE(led.showingTransientStatus);
}

void test_rgbShowConstantVoltageCharging_DrivesExpectedColor(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowConstantVoltageCharging(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 25), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 25), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedBlue);
    TEST_ASSERT_TRUE(led.showingTransientStatus);
}

void test_rgbShowDoneCharging_DrivesExpectedColor(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowDoneCharging(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedRed);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 25), capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(colorRangeToDuty(&led, 0), capturedBlue);
    TEST_ASSERT_TRUE(led.showingTransientStatus);
}

void test_rgbShowNoColor_DrivesZero(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowNoColor(&led);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(0, capturedRed);
    TEST_ASSERT_EQUAL_UINT16(0, capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(0, capturedBlue);
}

// ── Integration: rgbShowUserColor drives PWM correctly ──────────────

void test_rgbShowUserColor_Black_DrivesPwmToZero(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowUserColor(&led, 0, 0, 0);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(0, capturedRed);
    TEST_ASSERT_EQUAL_UINT16(0, capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(0, capturedBlue);
}

void test_rgbShowUserColor_White_DrivesPwmToMax_Period255(void) {
    rgbInit(&led, mock_writePwm, 255);
    rgbShowUserColor(&led, 255, 255, 255);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(255, capturedRed);
    TEST_ASSERT_EQUAL_UINT16(255, capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(255, capturedBlue);
}

void test_rgbShowUserColor_White_DrivesPwmToMax_Period511(void) {
    rgbInit(&led, mock_writePwm, 511);
    rgbShowUserColor(&led, 255, 255, 255);
    TEST_ASSERT_TRUE(writePwmCalled);
    TEST_ASSERT_EQUAL_UINT16(511, capturedRed);
    TEST_ASSERT_EQUAL_UINT16(511, capturedGreen);
    TEST_ASSERT_EQUAL_UINT16(511, capturedBlue);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_colorRangeToDuty_KnownPoint_Period255_Value15);
    RUN_TEST(test_colorRangeToDuty_KnownPoint_Period511_Value15);
    RUN_TEST(test_colorRangeToDuty_KnownPoint_Period511_Value25);
    RUN_TEST(test_colorRangeToDuty_MaxInput_ReturnsPeriod1);
    RUN_TEST(test_colorRangeToDuty_MaxInput_ReturnsPeriod100);
    RUN_TEST(test_colorRangeToDuty_MaxInput_ReturnsPeriod255);
    RUN_TEST(test_colorRangeToDuty_MaxInput_ReturnsPeriod511);
    RUN_TEST(test_colorRangeToDuty_Monotonic_Period255);
    RUN_TEST(test_colorRangeToDuty_Monotonic_Period511);
    RUN_TEST(test_colorRangeToDuty_ZeroInput_ReturnsZero);
    RUN_TEST(test_gammaLUT_Endpoints);
    RUN_TEST(test_gammaLUT_KnownPoints);
    RUN_TEST(test_gammaLUT_Monotonic);
    RUN_TEST(test_rgbInit_NullCallback_ReturnsFalse);
    RUN_TEST(test_rgbInit_NullDevice_ReturnsFalse);
    RUN_TEST(test_rgbInit_Period511_Accepted);
    RUN_TEST(test_rgbInit_PeriodAbove511_ReturnsFalse);
    RUN_TEST(test_rgbInit_ValidParams_SetsFieldsCorrectly);
    RUN_TEST(test_rgbShowConstantCurrentCharging_DrivesExpectedColor);
    RUN_TEST(test_rgbShowConstantVoltageCharging_DrivesExpectedColor);
    RUN_TEST(test_rgbShowDoneCharging_DrivesExpectedColor);
    RUN_TEST(test_rgbShowLocked_DrivesExpectedColor);
    RUN_TEST(test_rgbShowNoColor_DrivesZero);
    RUN_TEST(test_rgbShowNotCharging_DrivesExpectedColor);
    RUN_TEST(test_rgbShowShutdown_DrivesExpectedColor);
    RUN_TEST(test_rgbShowSuccess_DrivesExpectedColor);
    RUN_TEST(test_rgbShowUserColor_Black_DrivesPwmToZero);
    RUN_TEST(test_rgbShowUserColor_WhileTransient_StoresButDoesNotDrive);
    RUN_TEST(test_rgbShowUserColor_White_DrivesPwmToMax_Period255);
    RUN_TEST(test_rgbShowUserColor_White_DrivesPwmToMax_Period511);
    RUN_TEST(test_rgbTransientTask_NotTransient_NoChange);
    RUN_TEST(test_rgbTransientTask_NullDevice_NoOp);
    RUN_TEST(test_rgbTransientTask_TransientHeld_Before300ms);
    RUN_TEST(test_rgbTransientTask_TransientRevert_After300ms);
    return UNITY_END();
}
