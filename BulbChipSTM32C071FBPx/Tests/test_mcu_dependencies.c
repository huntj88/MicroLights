#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "unity.h"

// Include the mock headers
#include "mock_gpio_moder.h"
#include "stm32c0xx.h"
#include "stm32c0xx_hal.h"

// HAL peripheral instances required by mcu_dependencies.c
FLASH_TypeDef mockFlashPeripheral;
FLASH_TypeDef *FLASH = &mockFlashPeripheral;
TIM_TypeDef mockTIM1;
TIM_TypeDef mockTIM3;
RCC_TypeDef mockRCC = {.CR = RCC_CR_HSIUSB48RDY};
CRS_TypeDef mockCRS;
I2C_HandleTypeDef hi2c1;
RTC_HandleTypeDef hrtc;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim17;
PWR_TypeDef mockPWR;
uint16_t mockLastExtiClearedLine;

// GPIO mock instance for GPIOA — allows code to read/write MODER register.
GPIO_TypeDef mockGPIOA;

// I2C init tracking
static uint32_t i2cInitCallCount = 0;
static RTC_TimeTypeDef mockRtcTime;
static RTC_DateTypeDef mockRtcDate;
static RTC_AlarmTypeDef lastRtcAlarm;
static uint32_t rtcSetAlarmCallCount = 0;
static uint32_t rtcDeactivateAlarmCallCount = 0;
static uint32_t wakeUpPinEnableCallCount = 0;
static uint32_t wakeUpPinDisableCallCount = 0;
static uint32_t lastWakeUpPinEnable = 0;
static uint32_t lastWakeUpPinDisable = 0;
static uint32_t enterStopModeCallCount = 0;
static uint32_t enterStandbyModeCallCount = 0;
static uint32_t systemClockConfigCallCount = 0;

// GPIO init tracking
static GPIO_InitTypeDef lastGpioInit;
static GPIO_TypeDef *lastGpioPort = NULL;
static bool gpioInitCalled = false;

// GPIO write tracking
typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState state;
} GpioWriteRecord;

#define MAX_GPIO_WRITES 16
static GpioWriteRecord gpioWrites[MAX_GPIO_WRITES];
static uint32_t gpioWriteCount = 0;

// HAL stubs
HAL_StatusTypeDef HAL_I2C_Mem_Read(
    I2C_HandleTypeDef *hi2c,
    uint16_t DevAddress,
    uint16_t MemAddress,
    uint16_t MemAddSize,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout) {
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(
    I2C_HandleTypeDef *hi2c,
    uint16_t DevAddress,
    uint16_t MemAddress,
    uint16_t MemAddSize,
    uint8_t *pData,
    uint16_t Size,
    uint32_t Timeout) {
    return HAL_OK;
}
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin) {
    return GPIO_PIN_RESET;
}
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) {
    if (!GPIO_Init) {
        return;
    }
    lastGpioPort = GPIOx;
    lastGpioInit = *GPIO_Init;
    gpioInitCalled = true;
    mock_simulateModerUpdate(GPIOx, GPIO_Init);
}
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    if (gpioWriteCount < MAX_GPIO_WRITES) {
        gpioWrites[gpioWriteCount++] = (GpioWriteRecord){
            .port = GPIOx,
            .pin = GPIO_Pin,
            .state = PinState,
        };
    }
}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim) {
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim) {
    return HAL_OK;
}
void HAL_RCC_GetClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t *pFLatency) {
}
uint32_t HAL_RCC_GetPCLK1Freq(void) {
    return 1000000;
}
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c) {
    i2cInitCallCount++;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_RTC_GetTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *sTime, uint32_t Format) {
    (void)hrtc;
    (void)Format;
    *sTime = mockRtcTime;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_RTC_GetDate(
    const RTC_HandleTypeDef *hrtc, RTC_DateTypeDef *sDate, uint32_t Format) {
    (void)hrtc;
    (void)Format;
    *sDate = mockRtcDate;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_RTC_SetAlarm_IT(
    RTC_HandleTypeDef *hrtc, RTC_AlarmTypeDef *sAlarm, uint32_t Format) {
    (void)hrtc;
    (void)Format;
    rtcSetAlarmCallCount++;
    lastRtcAlarm = *sAlarm;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_RTC_DeactivateAlarm(RTC_HandleTypeDef *hrtc, uint32_t Alarm) {
    (void)hrtc;
    (void)Alarm;
    rtcDeactivateAlarmCallCount++;
    return HAL_OK;
}
void HAL_PWR_EnableWakeUpPin(uint32_t WakeUpPinPolarity) {
    wakeUpPinEnableCallCount++;
    lastWakeUpPinEnable = WakeUpPinPolarity;
}
void HAL_PWR_DisableWakeUpPin(uint32_t WakeUpPinx) {
    wakeUpPinDisableCallCount++;
    lastWakeUpPinDisable = WakeUpPinx;
}
HAL_StatusTypeDef HAL_PWREx_EnableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber) {
    (void)GPIO;
    (void)GPIONumber;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullUp(uint32_t GPIO, uint32_t GPIONumber) {
    (void)GPIO;
    (void)GPIONumber;
    return HAL_OK;
}
HAL_StatusTypeDef HAL_PWREx_DisableGPIOPullDown(uint32_t GPIO, uint32_t GPIONumber) {
    (void)GPIO;
    (void)GPIONumber;
    return HAL_OK;
}
void HAL_PWREx_EnablePullUpPullDownConfig(void) {
}
void HAL_PWREx_DisablePullUpPullDownConfig(void) {
}
void HAL_PWR_EnterSTOPMode(uint32_t Regulator, uint8_t STOPEntry) {
    (void)Regulator;
    (void)STOPEntry;
    enterStopModeCallCount++;
}
void HAL_PWR_EnterSTANDBYMode(void) {
    enterStandbyModeCallCount++;
}

void SystemClock_Config(void) {
    systemClockConfigCallCount++;
}

// TinyUSB stubs
static bool tudConnectCalled = false;
static bool tudDisconnectCalled = false;
bool tud_connect(void) {
    tudConnectCalled = true;
    return true;
}
bool tud_disconnect(void) {
    tudDisconnectCalled = true;
    return true;
}
bool tud_connected(void) {
    return false;
}

// Flash stubs — not under test here, just satisfying linker
HAL_StatusTypeDef HAL_FLASH_Unlock(void) {
    return HAL_OK;
}
HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout) {
    return HAL_OK;
}
void FLASH_PageErase(uint32_t Page) {
}
HAL_StatusTypeDef HAL_FLASH_Lock(void) {
    return HAL_OK;
}
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data) {
    return HAL_OK;
}
void __HAL_FLASH_CLEAR_FLAG(uint32_t flags) {
}

// Include the source files under test
#include "../Core/Src/flash_string.c"
#include "../Core/Src/mcu_dependencies.c"

void setUp(void) {
    memset(&lastGpioInit, 0, sizeof(lastGpioInit));
    lastGpioPort = NULL;
    gpioInitCalled = false;
    gpioWriteCount = 0;
    i2cInitCallCount = 0;
    tudConnectCalled = false;
    tudDisconnectCalled = false;
    memset(&mockRtcTime, 0, sizeof(mockRtcTime));
    mockRtcDate = (RTC_DateTypeDef){.WeekDay = 1, .Month = 1, .Date = 1, .Year = 0};
    memset(&lastRtcAlarm, 0, sizeof(lastRtcAlarm));
    rtcSetAlarmCallCount = 0;
    rtcDeactivateAlarmCallCount = 0;
    wakeUpPinEnableCallCount = 0;
    wakeUpPinDisableCallCount = 0;
    lastWakeUpPinEnable = 0;
    lastWakeUpPinDisable = 0;
    mockLastExtiClearedLine = 0;
    enterStopModeCallCount = 0;
    enterStandbyModeCallCount = 0;
    systemClockConfigCallCount = 0;
    memset(&mockPWR, 0, sizeof(mockPWR));
    mockRCC.CR = RCC_CR_HSIUSB48RDY;
    mockCRS.CR = 0;
    // Reset timer prescalers to a known sentinel so tests can detect changes
    htim1.Init.Prescaler = 0xFFFFFFFFU;
    htim3.Init.Prescaler = 0xFFFFFFFFU;
    // Set PA8 to GPIO output mode (MODER bits [17:16] = 0b01) — simulates
    // the pin state after CubeMX init, before any AF reconfiguration.
    mockGPIOA.MODER = (0x1U << (8U * 2U));
}

void tearDown(void) {
}

// Legacy: bulbLed_Pin is the old single-pin output. Once hardware migration is
// complete, these tests and the bulbLed_Pin writes in writeBulbLed() can be deleted.
void test_WriteBulbLed_Legacy_DrivesBothBulbAndFBluePins(void) {
    // fBlue starts in GPIO mode, so writeBulbLed should drive both pins
    TEST_ASSERT_FALSE(fBluePinIsAfMode());
    gpioWriteCount = 0;

    writeBulbLed(1);

    TEST_ASSERT_GREATER_OR_EQUAL_UINT32_MESSAGE(
        2, gpioWriteCount, "writeBulbLed should drive at least 2 GPIO pins in GPIO mode");

    bool bulbLedWritten = false;
    bool fBlueWritten = false;
    for (uint32_t i = 0; i < gpioWriteCount; i++) {
        if (gpioWrites[i].pin == bulbLed_Pin && gpioWrites[i].state == GPIO_PIN_SET) {
            bulbLedWritten = true;
        }
        if (gpioWrites[i].pin == fBlue_Pin && gpioWrites[i].state == GPIO_PIN_SET) {
            fBlueWritten = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(bulbLedWritten, "legacy bulbLed pin should be SET");
    TEST_ASSERT_TRUE_MESSAGE(fBlueWritten, "fBlue pin should be SET in GPIO mode");

    // Verify OFF state drives both pins low
    gpioWriteCount = 0;

    writeBulbLed(0);

    bulbLedWritten = false;
    fBlueWritten = false;
    for (uint32_t i = 0; i < gpioWriteCount; i++) {
        if (gpioWrites[i].pin == bulbLed_Pin && gpioWrites[i].state == GPIO_PIN_RESET) {
            bulbLedWritten = true;
        }
        if (gpioWrites[i].pin == fBlue_Pin && gpioWrites[i].state == GPIO_PIN_RESET) {
            fBlueWritten = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(bulbLedWritten, "legacy bulbLed pin should be RESET");
    TEST_ASSERT_TRUE_MESSAGE(fBlueWritten, "fBlue pin should be RESET in GPIO mode");
}

void test_EnableFrontLedTimer_GpioReconfigurationRoundTrip(void) {
    // Start in GPIO mode (default after reset)
    TEST_ASSERT_FALSE(fBluePinIsAfMode());

    // --- Step 1: enableFrontLedTimer(true) reconfigures to AF/PWM ---
    gpioInitCalled = false;
    enableFrontLedTimer(true);

    TEST_ASSERT_TRUE_MESSAGE(gpioInitCalled, "Should reconfigure to AF on first enable");
    TEST_ASSERT_EQUAL_UINT32(GPIO_MODE_AF_PP, lastGpioInit.Mode);
    TEST_ASSERT_TRUE(fBluePinIsAfMode());

    // --- Step 2: writeBulbLed does NOT reconfigure when pin is AF ---
    gpioInitCalled = false;
    gpioWriteCount = 0;
    writeBulbLed(1);

    TEST_ASSERT_FALSE_MESSAGE(gpioInitCalled, "Should NOT reconfigure GPIO when pin is AF");
    TEST_ASSERT_TRUE_MESSAGE(fBluePinIsAfMode(), "Pin should remain in AF mode");
    // fBlue should NOT be written when in AF mode
    bool fBlueWritten = false;
    for (uint32_t i = 0; i < gpioWriteCount; i++) {
        if (gpioWrites[i].pin == fBlue_Pin) {
            fBlueWritten = true;
        }
    }
    TEST_ASSERT_FALSE_MESSAGE(fBlueWritten, "fBlue should not be driven when in AF mode");

    // --- Step 3: enableFrontLedTimer(false) reconfigures to GPIO ---
    gpioInitCalled = false;
    enableFrontLedTimer(false);

    TEST_ASSERT_TRUE_MESSAGE(gpioInitCalled, "Should reconfigure to GPIO on disable");
    TEST_ASSERT_EQUAL_UINT32(GPIO_MODE_OUTPUT_PP, lastGpioInit.Mode);
    TEST_ASSERT_FALSE(fBluePinIsAfMode());
    gpioInitCalled = false;
    gpioWriteCount = 0;
    writeBulbLed(0);

    TEST_ASSERT_FALSE_MESSAGE(gpioInitCalled, "Should skip GPIO init when already in GPIO mode");
    fBlueWritten = false;
    for (uint32_t i = 0; i < gpioWriteCount; i++) {
        if (gpioWrites[i].pin == fBlue_Pin) {
            fBlueWritten = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(fBlueWritten, "fBlue should be driven in GPIO mode");
}

void test_FrontBluePin_ReconfiguresBetweenGpioAndPwm(void) {
    // Pin starts in GPIO mode after reset, so writeBulbLed should NOT reconfigure
    gpioInitCalled = false;
    writeBulbLed(1);
    TEST_ASSERT_FALSE_MESSAGE(gpioInitCalled, "Should skip GPIO init when already in GPIO mode");

    // Transition to AF/PWM mode — should reconfigure
    gpioInitCalled = false;
    memset(&lastGpioInit, 0, sizeof(lastGpioInit));

    enableFrontLedTimer(true);

    TEST_ASSERT_TRUE(gpioInitCalled);
    TEST_ASSERT_EQUAL_UINT16(fBlue_Pin, lastGpioInit.Pin);
    TEST_ASSERT_EQUAL_UINT32(GPIO_MODE_AF_PP, lastGpioInit.Mode);
    TEST_ASSERT_EQUAL_UINT32(GPIO_AF12_TIM3, lastGpioInit.Alternate);

    // Calling enableFrontLedTimer(true) again should NOT reconfigure
    gpioInitCalled = false;
    enableFrontLedTimer(true);
    TEST_ASSERT_FALSE_MESSAGE(gpioInitCalled, "Should skip AF init when already in AF mode");

    // writeBulbLed should NOT reconfigure when in AF mode
    gpioInitCalled = false;
    writeBulbLed(1);
    TEST_ASSERT_FALSE_MESSAGE(
        gpioInitCalled, "writeBulbLed must not reconfigure fBlue when in AF mode");
    TEST_ASSERT_TRUE_MESSAGE(fBluePinIsAfMode(), "Pin should still be AF after writeBulbLed");

    // enableFrontLedTimer(false) reconfigures back to GPIO
    gpioInitCalled = false;
    memset(&lastGpioInit, 0, sizeof(lastGpioInit));

    enableFrontLedTimer(false);

    TEST_ASSERT_TRUE(gpioInitCalled);
    TEST_ASSERT_EQUAL_UINT16(fBlue_Pin, lastGpioInit.Pin);
    TEST_ASSERT_EQUAL_UINT32(GPIO_MODE_OUTPUT_PP, lastGpioInit.Mode);

    // Calling writeBulbLed again should NOT reconfigure (already GPIO)
    gpioInitCalled = false;
    writeBulbLed(0);
    TEST_ASSERT_FALSE_MESSAGE(gpioInitCalled, "Should skip GPIO init when already in GPIO mode");
}

void test_EnableUsbClock_Enable_SetsUpClocksAndI2C(void) {
    mockRCC.CR = RCC_CR_HSIUSB48RDY;  // HSI48 ready bit already set so while-loop exits
    mockCRS.CR = 0;
    hi2c1.Init.Timing = 0;

    enableUsbClock(true);

    // HSI48 should be enabled
    TEST_ASSERT_BITS_HIGH_MESSAGE(RCC_CR_HSIUSB48ON, mockRCC.CR, "HSI48 should be enabled");
    // CRS auto-trim and enable bits should be set
    TEST_ASSERT_BITS_HIGH_MESSAGE(
        CRS_CR_AUTOTRIMEN | CRS_CR_CEN, mockCRS.CR, "CRS should have AUTOTRIMEN and CEN set");
    // I2C timing should be reconfigured for 48 MHz
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(
        I2C_TIMING_48MHZ, hi2c1.Init.Timing, "I2C timing should be set for 48 MHz");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, i2cInitCallCount, "HAL_I2C_Init should be called once");
    // PWM prescalers should be updated for 48 MHz (constant PWM frequency)
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        PWM_PRESCALER_48MHZ, htim1.Init.Prescaler, "TIM1 prescaler should be set for 48 MHz");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        PWM_PRESCALER_48MHZ, htim3.Init.Prescaler, "TIM3 prescaler should be set for 48 MHz");
    // tud_connect should be called
    TEST_ASSERT_TRUE_MESSAGE(tudConnectCalled, "tud_connect should be called");
    TEST_ASSERT_FALSE_MESSAGE(tudDisconnectCalled, "tud_disconnect should NOT be called");
}

void test_EnableUsbClock_Disable_TearsDownClocksAndI2C(void) {
    // First enable so there's something to tear down
    mockRCC.CR = RCC_CR_HSIUSB48RDY;
    enableUsbClock(true);
    // Reset tracking
    i2cInitCallCount = 0;
    tudConnectCalled = false;
    tudDisconnectCalled = false;

    enableUsbClock(false);

    // HSI48 should be disabled
    TEST_ASSERT_BITS_LOW_MESSAGE(RCC_CR_HSIUSB48ON, mockRCC.CR, "HSI48 should be disabled");
    // CRS bits should be cleared
    TEST_ASSERT_BITS_LOW_MESSAGE(
        CRS_CR_AUTOTRIMEN | CRS_CR_CEN, mockCRS.CR, "CRS AUTOTRIMEN and CEN should be cleared");
    // I2C timing should be reconfigured for 12 MHz
    TEST_ASSERT_EQUAL_HEX32_MESSAGE(
        I2C_TIMING_12MHZ, hi2c1.Init.Timing, "I2C timing should be set for 12 MHz");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        1, i2cInitCallCount, "HAL_I2C_Init should be called once on disable");
    // PWM prescalers should be restored for 12 MHz (constant PWM frequency)
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        PWM_PRESCALER_12MHZ, htim1.Init.Prescaler, "TIM1 prescaler should be set for 12 MHz");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        PWM_PRESCALER_12MHZ, htim3.Init.Prescaler, "TIM3 prescaler should be set for 12 MHz");
    // tud_disconnect should be called
    TEST_ASSERT_TRUE_MESSAGE(tudDisconnectCalled, "tud_disconnect should be called");
    TEST_ASSERT_FALSE_MESSAGE(tudConnectCalled, "tud_connect should NOT be called on disable");
}

void test_EnableUsbClock_InvalidatesTickMultiplier(void) {
    // Prime the multiplier cache by calling convertTicksToMilliseconds
    htim2.Init.Prescaler = 47;
    htim2.Init.Period = 999;
    uint32_t ms1 = convertTicksToMilliseconds(100);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
        0, tickMultiplier, "tickMultiplier should be cached after first call");

    // Enable USB clock — should reset the multiplier
    mockRCC.CR = RCC_CR_HSIUSB48RDY;
    enableUsbClock(true);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, tickMultiplier, "tickMultiplier should be reset after enableUsbClock(true)");

    // Re-prime the cache
    ms1 = convertTicksToMilliseconds(100);
    (void)ms1;
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, tickMultiplier, "tickMultiplier should be cached again");

    // Disable USB clock — should reset the multiplier again
    enableUsbClock(false);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
        0, tickMultiplier, "tickMultiplier should be reset after enableUsbClock(false)");
}

void test_EnterStandbyMode_ConfiguresWakePinAndClearsFlags(void) {
    enterStandbyMode();

    TEST_ASSERT_EQUAL_UINT32(1, wakeUpPinDisableCallCount);
    TEST_ASSERT_EQUAL_UINT32(PWR_WAKEUP_PIN2, lastWakeUpPinDisable);
    TEST_ASSERT_EQUAL_UINT32(1, wakeUpPinEnableCallCount);
    TEST_ASSERT_EQUAL_UINT32(PWR_WAKEUP_PIN2_LOW, lastWakeUpPinEnable);
    TEST_ASSERT_EQUAL_UINT32(PWR_FLAG_WUF | PWR_FLAG_SB, mockPWR.SCR);
    TEST_ASSERT_EQUAL_UINT32(1, enterStandbyModeCallCount);
}

void test_EnterStopModeWithRtcAlarm_SchedulesAlarmAndRestoresClock(void) {
    mockRtcTime = (RTC_TimeTypeDef){.Hours = 1, .Minutes = 2, .Seconds = 3};
    mockRtcDate = (RTC_DateTypeDef){.WeekDay = 1, .Month = 1, .Date = 1, .Year = 0};

    enterStopModeWithRtcAlarm(60);

    TEST_ASSERT_EQUAL_UINT32(1, wakeUpPinDisableCallCount);
    TEST_ASSERT_EQUAL_UINT32(PWR_WAKEUP_PIN2, lastWakeUpPinDisable);
    TEST_ASSERT_EQUAL_UINT32(1, wakeUpPinEnableCallCount);
    TEST_ASSERT_EQUAL_UINT32(PWR_WAKEUP_PIN2_LOW, lastWakeUpPinEnable);
    TEST_ASSERT_EQUAL_UINT32(PWR_FLAG_WUF | PWR_FLAG_SB, mockPWR.SCR);
    TEST_ASSERT_EQUAL_UINT32(1, rtcSetAlarmCallCount);
    TEST_ASSERT_EQUAL_UINT8(1, lastRtcAlarm.AlarmTime.Hours);
    TEST_ASSERT_EQUAL_UINT8(3, lastRtcAlarm.AlarmTime.Minutes);
    TEST_ASSERT_EQUAL_UINT8(3, lastRtcAlarm.AlarmTime.Seconds);
    TEST_ASSERT_EQUAL_UINT8(1, lastRtcAlarm.AlarmDateWeekDay);
    TEST_ASSERT_EQUAL_UINT32(2, rtcDeactivateAlarmCallCount);
    TEST_ASSERT_EQUAL_UINT32(1, enterStopModeCallCount);
    TEST_ASSERT_EQUAL_UINT32(1, systemClockConfigCallCount);
}

void test_WasWakeFromButton_ReturnsTrueAndClearsFlag(void) {
    mockPWR.SR1 = PWR_SR1_WUF2;

    TEST_ASSERT_TRUE(wasWakeFromButton());
    TEST_ASSERT_EQUAL_UINT32(PWR_FLAG_WUF2, mockPWR.SCR);

    mockPWR.SR1 = 0;
    TEST_ASSERT_FALSE(wasWakeFromButton());
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_EnableFrontLedTimer_GpioReconfigurationRoundTrip);
    RUN_TEST(test_EnableUsbClock_Disable_TearsDownClocksAndI2C);
    RUN_TEST(test_EnableUsbClock_Enable_SetsUpClocksAndI2C);
    RUN_TEST(test_EnableUsbClock_InvalidatesTickMultiplier);
    RUN_TEST(test_EnterStandbyMode_ConfiguresWakePinAndClearsFlags);
    RUN_TEST(test_EnterStopModeWithRtcAlarm_SchedulesAlarmAndRestoresClock);
    RUN_TEST(test_FrontBluePin_ReconfiguresBetweenGpioAndPwm);
    RUN_TEST(test_WasWakeFromButton_ReturnsTrueAndClearsFlag);
    RUN_TEST(test_WriteBulbLed_Legacy_DrivesBothBulbAndFBluePins);
    return UNITY_END();
}
