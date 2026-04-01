/*
 * mcu_dependencies.c
 *
 *  Created on: Jan 8, 2026
 *      Author: jameshunt
 */

#include "mcu_dependencies.h"
#include "main.h"
#include "tusb.h"

extern I2C_HandleTypeDef hi2c1;
extern RTC_HandleTypeDef hrtc;
extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim17;
extern void SystemClock_Config(void);

#define SETTINGS_PAGE 56  // 2K flash reserved for settings starting at page 56
#define BULB_PAGE_0 57    // 14K flash reserved for bulb modes starting at page 57

// fBlue pin is shared between GPIO (bulb) and AF (TIM3 PWM) modes.
// Read the hardware MODER register to detect current pin configuration
// and avoid expensive HAL_GPIO_Init on every call.
// PA8 → MODER bits [17:16]: 0b00 = Input, 0b01 = Output, 0b10 = AF, 0b11 = Analog
#define FBLUE_PIN_POS 8U
#define FBLUE_MODER_MASK (0x3U << (FBLUE_PIN_POS * 2U))
#define FBLUE_MODER_AF (0x2U << (FBLUE_PIN_POS * 2U))

#define I2C_TIMING_48MHZ 0x10805D88U
#define I2C_TIMING_12MHZ 0x00402D41U

// PWM timer prescaler values to maintain ~8 kHz PWM across clock speeds.
// 12 MHz / (2+1) / (500+1) ≈ 7984 Hz
// 48 MHz / (11+1) / (500+1) ≈ 7984 Hz
#define PWM_PRESCALER_12MHZ 2U
#define PWM_PRESCALER_48MHZ 11U

#define BUTTON_WAKEUP_PIN PWR_WAKEUP_PIN2_LOW
#define BUTTON_WAKEUP_PIN_MASK PWR_WAKEUP_PIN2
#define BUTTON_WAKEUP_FLAG PWR_FLAG_WUF2

// Cached tick-to-millisecond multiplier (file scope so enableUsbClock can invalidate it)
static uint32_t tickMultiplier = 0;

static bool requireHalOk(HAL_StatusTypeDef status) {
    if (status != HAL_OK) {
        Error_Handler();
        return false;
    }
    return true;
}

static bool scheduleRtcAlarmInSeconds(uint16_t wakeIntervalSeconds) {
    RTC_TimeTypeDef currentTime = {0};
    RTC_DateTypeDef currentDate = {0};
    RTC_AlarmTypeDef alarm = {0};

    if (!requireHalOk(HAL_RTC_GetTime(&hrtc, &currentTime, RTC_FORMAT_BIN)) ||
        !requireHalOk(HAL_RTC_GetDate(&hrtc, &currentDate, RTC_FORMAT_BIN))) {
        return false;
    }

    uint32_t totalSeconds = (uint32_t)currentTime.Hours * 3600U +
                            (uint32_t)currentTime.Minutes * 60U + (uint32_t)currentTime.Seconds +
                            (uint32_t)wakeIntervalSeconds;
    uint8_t alarmDate = currentDate.Date;

    // We only care about relative wake intervals while the chip remains in stop mode.
    // Alarm intervals are intentionally short and wakeIntervalSeconds is uint16_t,
    // so the rollover can only ever stay on the current day or move to the next day.
    // We intentionally do not handle month/year rollover here.
    if (totalSeconds >= 86400U) {
        totalSeconds -= 86400U;
        alarmDate++;
    }

    uint8_t alarmHours = (uint8_t)(totalSeconds / 3600U);
    totalSeconds %= 3600U;
    uint8_t alarmMinutes = (uint8_t)(totalSeconds / 60U);
    uint8_t alarmSeconds = (uint8_t)(totalSeconds % 60U);

    alarm.AlarmTime.Hours = alarmHours;
    alarm.AlarmTime.Minutes = alarmMinutes;
    alarm.AlarmTime.Seconds = alarmSeconds;
    alarm.AlarmTime.SubSeconds = 0;
    alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
    alarm.AlarmMask = RTC_ALARMMASK_NONE;
    alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    alarm.AlarmDateWeekDay = alarmDate;
    alarm.Alarm = RTC_ALARM_A;

    if (!requireHalOk(HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A))) {
        return false;
    }
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
    return requireHalOk(HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN));
}

static inline bool fBluePinIsAfMode(void) {
    return (fBlue_GPIO_Port->MODER & FBLUE_MODER_MASK) == FBLUE_MODER_AF;
}

static void configureFrontBlueGpio(void) {
    if (!fBluePinIsAfMode()) {
        return;
    }
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = fBlue_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(fBlue_GPIO_Port, &GPIO_InitStruct);
}

static void configureFrontBluePwm(void) {
    if (fBluePinIsAfMode()) {
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
    GPIO_PinState pinState = (state == 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    // TODO: remove legacy bulbLed pin once hardware migration is complete.
    HAL_GPIO_WritePin(bulbLed_GPIO_Port, bulbLed_Pin, pinState);
    // Only drive fBlue when pin is in GPIO mode. When the front LED timer owns
    // the pin (AF/PWM mode), writing GPIO would have no visible effect but
    // reconfiguring to GPIO would break the PWM output.
    if (!fBluePinIsAfMode()) {
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
    // HAL_TIM_PWM Start/Stop expand to identical-looking branches (lint false positive)
    // NOLINTNEXTLINE(bugprone-branch-clone)
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

void enableUsbClock(bool enable) {
    if (enable) {
        __HAL_RCC_HSI48_ENABLE();
        while (READ_BIT(RCC->CR, RCC_CR_HSIUSB48RDY) == 0U) {
        }
        __HAL_RCC_CRS_CLK_ENABLE();
        CRS->CR |= CRS_CR_AUTOTRIMEN | CRS_CR_CEN;
        __HAL_RCC_USB_CLK_ENABLE();

        // Boost SYSCLK to 48 MHz for faster USB enumeration.
        // Increase flash latency before raising clock speed.
        __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);
        MODIFY_REG(RCC->CR, RCC_CR_HSIDIV, RCC_HSI_DIV1);
        SystemCoreClockUpdate();
        HAL_InitTick(uwTickPrio);
        tickMultiplier = 0;

        hi2c1.Init.Timing = I2C_TIMING_48MHZ;
        HAL_I2C_Init(&hi2c1);

        // Scale PWM prescaler to keep frequency constant at ~8 kHz.
        // PSC is shadow-registered: new value loads at next counter overflow (glitch-free).
        __HAL_TIM_SET_PRESCALER(&htim1, PWM_PRESCALER_48MHZ);
        __HAL_TIM_SET_PRESCALER(&htim3, PWM_PRESCALER_48MHZ);

        tud_connect();
    } else {
        tud_disconnect();

        // Drop SYSCLK back to 12 MHz to save power.
        // Reduce clock speed before lowering flash latency.
        MODIFY_REG(RCC->CR, RCC_CR_HSIDIV, RCC_HSI_DIV4);
        __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);
        SystemCoreClockUpdate();
        HAL_InitTick(uwTickPrio);
        tickMultiplier = 0;

        hi2c1.Init.Timing = I2C_TIMING_12MHZ;
        HAL_I2C_Init(&hi2c1);

        // Restore PWM prescaler for 12 MHz clock speed.
        __HAL_TIM_SET_PRESCALER(&htim1, PWM_PRESCALER_12MHZ);
        __HAL_TIM_SET_PRESCALER(&htim3, PWM_PRESCALER_12MHZ);

        __HAL_RCC_USB_CLK_DISABLE();
        CRS->CR &= ~(CRS_CR_AUTOTRIMEN | CRS_CR_CEN);
        __HAL_RCC_CRS_CLK_DISABLE();
        __HAL_RCC_HSI48_DISABLE();
    }
}

void enableAutoOffTimer(bool enable) {
    if (enable) {
        HAL_TIM_Base_Start_IT(&htim17);
    } else {
        HAL_TIM_Base_Stop_IT(&htim17);
    }
}

void enterStandbyMode(void) {
    HAL_PWR_DisableWakeUpPin(BUTTON_WAKEUP_PIN_MASK);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF | PWR_FLAG_SB);
    HAL_PWR_EnableWakeUpPin(BUTTON_WAKEUP_PIN);

    HAL_SuspendTick();
    HAL_PWR_EnterSTANDBYMode();
    HAL_ResumeTick();
}

void enterStopModeWithRtcAlarm(uint16_t wakeIntervalSeconds) {
#ifdef MICROLIGHT_LEGACY_PCB_BUTTON_PA7
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF | PWR_FLAG_SB);
#else
    HAL_PWR_DisableWakeUpPin(BUTTON_WAKEUP_PIN_MASK);
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF | PWR_FLAG_SB);
    HAL_PWR_EnableWakeUpPin(BUTTON_WAKEUP_PIN);
#endif

    if (!scheduleRtcAlarmInSeconds(wakeIntervalSeconds)) {
        return;
    }

    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFI);
    SystemClock_Config();
    HAL_ResumeTick();

    if (!requireHalOk(HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A))) {
        return;
    }
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
}

static bool wasWakeFromButton(void) {
#ifdef MICROLIGHT_LEGACY_PCB_BUTTON_PA7
    return readButtonPin() == 0U;
#else
    bool didWakeFromButton = (__HAL_PWR_GET_FLAG(BUTTON_WAKEUP_FLAG) != 0U);
    if (didWakeFromButton) {
        __HAL_PWR_CLEAR_FLAG(BUTTON_WAKEUP_FLAG);
    }
    return didWakeFromButton;
#endif
}

bool waitForButtonWakeOrAutoLock(uint16_t wakeIntervalSeconds, uint16_t lockThresholdMinutes) {
    uint32_t elapsedSeconds = 0;
    uint32_t lockThresholdSeconds = (uint32_t)lockThresholdMinutes * 60U;

    while (true) {
        enterStopModeWithRtcAlarm(wakeIntervalSeconds);

        if (wasWakeFromButton()) {
            return true;
        }

        elapsedSeconds += wakeIntervalSeconds;
        if (elapsedSeconds >= lockThresholdSeconds) {
            return false;
        }
    }
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
    if (tickMultiplier == 0) {
        tickMultiplier = calculateTickMultiplier();
    }
    return (uint32_t)(((uint64_t)ticks * tickMultiplier) >> 20);
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

// Blink case LED white forever using direct register writes.
// Safe to call from any fault context — requires only that TIM1 is running.
// Uses a busy-loop delay since HAL/SysTick state cannot be trusted.
// Blink period is ~150 ms on at 48 MHz, ~600 ms at 12 MHz (cosmetic only).
#define BLINK_DELAY_LOOPS 300000U
__attribute__((noreturn)) void blinkCaseLedWhiteForever(void) {
    __disable_irq();

    if (!(TIM1->CR1 & TIM_CR1_CEN)) {
        while (1) {
        }  // TIM1 not running, just hang
    }

    volatile uint32_t delay;
    while (1) {
        TIM1->CCR1 = TIM1->ARR;
        TIM1->CCR2 = TIM1->ARR;
        TIM1->CCR3 = TIM1->ARR;
        for (delay = 0; delay < BLINK_DELAY_LOOPS; delay++) {
        }
        TIM1->CCR1 = 0;
        TIM1->CCR2 = 0;
        TIM1->CCR3 = 0;
        for (delay = 0; delay < BLINK_DELAY_LOOPS; delay++) {
        }
    }
}

// Override newlib's assert handler to blink white on the case LED before halting.
// Called by assert() when the expression is false (debug builds only).
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) — fixed newlib signature
void __assert_func(const char *file, int line, const char *func, const char *failedexpr) {
    (void)file;
    (void)line;
    (void)func;
    (void)failedexpr;
    blinkCaseLedWhiteForever();
}
