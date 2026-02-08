/*
 * mcu_dependencies.c
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include "mcu_dependencies.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c1;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim17;

#define SETTINGS_PAGE 56  // 2K flash reserved for settings starting at page 56
#define BULB_PAGE_0 57    // 14K flash reserved for bulb modes starting at page 57

// fBlue pin is shared between GPIO (bulb) and AF (TIM3 PWM) modes.
// Track current mode to avoid expensive HAL_GPIO_Init on every call.
static bool fBluePinIsAfMode = false;

static void configureFrontBlueGpio(void) {
    if (!fBluePinIsAfMode) {
        return;
    }
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = fBlue_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(fBlue_GPIO_Port, &GPIO_InitStruct);
    fBluePinIsAfMode = false;
}

static void configureFrontBluePwm(void) {
    if (fBluePinIsAfMode) {
        return;
    }
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = fBlue_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF12_TIM3;
    HAL_GPIO_Init(fBlue_GPIO_Port, &GPIO_InitStruct);
    fBluePinIsAfMode = true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool i2cWriteRegister(uint8_t devAddress, uint8_t reg, uint8_t value) {
    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Write(&hi2c1, devAddress, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 1000);

    return status == HAL_OK;
}

// Read multiple consecutive registers. Used with MC3479 (for efficient axis reads)
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
bool i2cReadRegisters(uint8_t devAddress, uint8_t startReg, uint8_t *buf, size_t len) {
    HAL_StatusTypeDef status =
        HAL_I2C_Mem_Read(&hi2c1, devAddress, startReg, I2C_MEMADD_SIZE_8BIT, buf, len, 1000);

    return status == HAL_OK;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void writeRgbPwmCaseLed(uint16_t redDuty, uint16_t greenDuty, uint16_t blueDuty) {
    TIM1->CCR1 = redDuty;
    TIM1->CCR2 = greenDuty;
    TIM1->CCR3 = blueDuty;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void writeRgbPwmFrontLed(uint16_t redDuty, uint16_t greenDuty, uint16_t blueDuty) {
    TIM3->CCR2 = redDuty;
    TIM3->CCR3 = greenDuty;
    TIM3->CCR4 = blueDuty;
}

uint8_t readButtonPin(void) {
    GPIO_PinState state = HAL_GPIO_ReadPin(button_GPIO_Port, button_Pin);
    if (state == GPIO_PIN_RESET) {
        return 0;
    }
    return 1;
}

void writeBulbLed(uint8_t state) {
    // Skip redundant HAL writes when the pin is already in the requested state.
    static uint8_t lastState = 255;  // sentinel: guarantees first write always reaches hardware
    if (state == lastState) {
        return;
    }
    lastState = state;

    GPIO_PinState pinState = (state == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    // TODO: remove legacy bulbLed pin once hardware migration is complete.
    HAL_GPIO_WritePin(bulbLed_GPIO_Port, bulbLed_Pin, pinState);
    // Only drive fBlue when pin is in GPIO mode. When the front LED timer owns
    // the pin (AF/PWM mode), writing GPIO would have no visible effect but
    // reconfiguring to GPIO would break the PWM output.
    if (!fBluePinIsAfMode) {
        HAL_GPIO_WritePin(fBlue_GPIO_Port, fBlue_Pin, pinState);
    }
}

void enableChipTickTimer(bool enable) {
    if (enable) {
        HAL_TIM_Base_Start_IT(&htim2);
    } else {
        HAL_TIM_Base_Stop_IT(&htim2);
    }
}

void enableCaseLedTimer(bool enable) {
    if (enable) {
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
    } else {
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_2);
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_3);

        HAL_GPIO_WritePin(red_GPIO_Port, red_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(green_GPIO_Port, green_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(blue_GPIO_Port, blue_Pin, GPIO_PIN_RESET);
    }
}

void enableFrontLedTimer(bool enable) {
    if (enable) {
        configureFrontBluePwm();
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);
    } else {
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_2);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_4);

        HAL_GPIO_WritePin(fRed_GPIO_Port, fRed_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(fGreen_GPIO_Port, fGreen_Pin, GPIO_PIN_RESET);
        configureFrontBlueGpio();
        HAL_GPIO_WritePin(fBlue_GPIO_Port, fBlue_Pin, GPIO_PIN_RESET);
    }
}

void startAutoOffTimer(void) {
    HAL_TIM_Base_Start_IT(&htim17);
}

static uint32_t calculateTickMultiplier(void) {
    RCC_ClkInitTypeDef clkConfig;
    uint32_t flashLatency;
    HAL_RCC_GetClockConfig(&clkConfig, &flashLatency);

    uint32_t timerClock = HAL_RCC_GetPCLK1Freq();
    if (clkConfig.APB1CLKDivider == RCC_HCLK_DIV2) {
        timerClock *= 2U;
    } else if (clkConfig.APB1CLKDivider == RCC_HCLK_DIV4) {
        timerClock *= 4U;
    } else {
        // TODO: more clock dividers
    }

    uint32_t prescaler = htim2.Init.Prescaler + 1U;
    uint32_t period = htim2.Init.Period + 1U;

    if (timerClock == 0U) {
        return 0;
    }

    // We want to calculate: multiplier = (msPerTick) * 2^20
    // msPerTick = (prescaler * period * 1000) / timerClock
    // multiplier = (prescaler * period * 1000 * 2^20) / timerClock
    // 2^20 = 1048576
    // 1000 * 1048576 = 1048576000

    // Use uint64_t to prevent overflow during calculation
    uint64_t numerator = (uint64_t)prescaler * (uint64_t)period * 1048576000ULL;
    return (uint32_t)(numerator / timerClock);
}

/**
 * @brief Converts chip ticks to milliseconds using fixed-point arithmetic optimization.
 *
 * Traditional approach:
 *   ms = (int)ticks * (float)millisecondsPerTick
 *   or
 *   ms = ((int)ticks * (int)microsecondsPerTick) / 1000
 *
 * This requires either floating point math (slow/large code size) or integer division (slow).
 *
 * Optimization:
 *   We use a fixed-point multiplier scaled by 2^20 (1048576).
 *   multiplier = millisecondsPerTick * 2^20
 *
 *   Calculation becomes:
 *   ms = (ticks * multiplier) / 2^20
 *
 *   Division by 2^20 is implemented as a right bit shift (>> 20), which is extremely fast.
 *   We use uint64_t for the intermediate multiplication to prevent overflow before the shift.
 *   2^20 is chosen to provide sufficient precision while keeping the multiplier within uint32_t
 *   (assuming millisecondsPerTick < ~4000ms) and the intermediate result within uint64_t.
 */
uint32_t convertTicksToMilliseconds(uint32_t ticks) {
    static uint32_t multiplier = 0;
    if (multiplier == 0) {
        multiplier = calculateTickMultiplier();
    }
    return (uint32_t)(((uint64_t)ticks * multiplier) >> 20);
}

void writeSettingsToFlash(const char str[], size_t length) {
    writeStringToFlash(SETTINGS_PAGE, str, length);
}

void readSettingsFromFlash(char buffer[], size_t length) {
    readStringFromFlash(SETTINGS_PAGE, buffer, length);
}

void writeModeToFlash(uint8_t mode, const char str[], size_t length) {
    uint32_t page = BULB_PAGE_0 + mode;
    writeStringToFlash(page, str, length);
}

void readModeFromFlash(uint8_t mode, char buffer[], size_t length) {
    uint32_t page = BULB_PAGE_0 + mode;
    readStringFromFlash(page, buffer, length);
}
