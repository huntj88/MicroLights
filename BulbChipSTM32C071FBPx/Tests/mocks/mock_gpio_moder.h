/*
 * mock_gpio_moder.h
 *
 * Shared MODER register simulation for tests that include mcu_dependencies.c.
 * Call mock_simulateModerUpdate() from your HAL_GPIO_Init mock to keep the
 * simulated MODER register in sync with HAL_GPIO_Init calls so that
 * fBluePinIsAfMode() returns the correct value.
 */

#ifndef MOCK_GPIO_MODER_H_
#define MOCK_GPIO_MODER_H_

#include "stm32c0xx_hal.h"

static inline void mock_simulateModerUpdate(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) {
    if (GPIOx && GPIO_Init && GPIO_Init->Pin) {
        uint16_t pin = GPIO_Init->Pin;
        uint32_t pos = 0;
        while ((pin >> pos) != 1U) {
            pos++;
        }
        uint32_t moder_val = GPIO_Init->Mode & 0x3U;
        GPIOx->MODER = (GPIOx->MODER & ~(0x3U << (pos * 2U))) | (moder_val << (pos * 2U));
    }
}

#endif /* MOCK_GPIO_MODER_H_ */
