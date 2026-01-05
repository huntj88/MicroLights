#ifndef STM32C0xx_H
#define STM32C0xx_H

#include <stdint.h>

typedef enum {
    HAL_OK = 0x00U,
    HAL_ERROR = 0x01U,
    HAL_BUSY = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct {
    volatile uint32_t CR;
} FLASH_TypeDef;

extern FLASH_TypeDef *FLASH;

#define FLASH_FLAG_EOP 0x00000001U
#define FLASH_FLAG_WRPERR 0x00000002U
#define FLASH_FLAG_PGAERR 0x00000004U

#define FLASH_CR_PER 0x00000002U

#define TYPEPROGRAM_DOUBLEWORD 0x00000003U

#define CLEAR_BIT(REG, BIT) ((REG) &= ~(BIT))

// Mock functions
HAL_StatusTypeDef HAL_FLASH_Unlock(void);
HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout);
void FLASH_PageErase(uint32_t Page);
HAL_StatusTypeDef HAL_FLASH_Lock(void);
HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data);

// Mock macro for __HAL_FLASH_CLEAR_FLAG
// In real code this clears flags. In mock we can make it a function or empty.
// storage.c uses: __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR);
void __HAL_FLASH_CLEAR_FLAG(uint32_t flags);

// Interrupt macros used in storage.c
#define __disable_irq()
#define __enable_irq()

#endif
