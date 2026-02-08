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
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim17;

// GPIO mock instance for GPIOA — allows code to read/write MODER register.
GPIO_TypeDef mockGPIOA;

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_EnableFrontLedTimer_GpioReconfigurationRoundTrip);
    RUN_TEST(test_FrontBluePin_ReconfiguresBetweenGpioAndPwm);
    RUN_TEST(test_WriteBulbLed_Legacy_DrivesBothBulbAndFBluePins);
    return UNITY_END();
}
