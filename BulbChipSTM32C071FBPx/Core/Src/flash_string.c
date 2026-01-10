#include "flash_string.h"
#include <string.h>
#include "stm32c0xx.h"

// Erase a memory page from the flash retrieve
static void memoryPageErase(uint32_t memoryPage) {
    HAL_StatusTypeDef eraseHandler = HAL_FLASH_Unlock();

    eraseHandler = FLASH_WaitForLastOperation(500);
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR);
    FLASH_PageErase(memoryPage);

    eraseHandler = FLASH_WaitForLastOperation(500);
    CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
    HAL_FLASH_Lock();
}

static uint32_t getHexAddressOfPage(uint32_t dataPage) {
    uint32_t bits = PAGE_SECTOR * dataPage;
    uint32_t hexAddress = FLASH_INIT + bits;
    return hexAddress;
}

void readStringFromFlash(uint32_t page, char buffer[], size_t length) {
    uint32_t address = getHexAddressOfPage(page);
    memcpy(buffer, (const void *)(uintptr_t)address, length);
    // Ensure null termination
    if (length > 0) {
        buffer[length - 1] = '\0';
    }
    // Check for erased flash (0xFF) and treat as empty string
    if ((unsigned char)buffer[0] == 0xFF) {
        buffer[0] = '\0';
    }
}

void writeStringToFlash(uint32_t page, const char str[], size_t length) {
    uint8_t numBytesToWrite = 8;

    __disable_irq();
    memoryPageErase(page);
    HAL_FLASH_Unlock();

    // Leave room for null terminator
    if (length >= PAGE_SECTOR) {
        length = PAGE_SECTOR - 1;
    }
    uint8_t emptyPaddingLength = numBytesToWrite - (length % numBytesToWrite);
    uint8_t dataSpaceBuf[numBytesToWrite];

    // Write 8 bytes at a time, pad with \0 at end.
    // If bytes count is not a multiple of 8, pad the remaining bytes with \0.
    for (size_t i = 0; i < length + emptyPaddingLength; i++) {
        if (i < length) {
            dataSpaceBuf[i % numBytesToWrite] = str[i];
        } else {
            dataSpaceBuf[i % numBytesToWrite] = '\0';
        }

        if (i % 8 == 8 - 1) {
            uint32_t address =
                getHexAddressOfPage(page) + ((numBytesToWrite * (i / numBytesToWrite)));

            uint64_t data;
            uint64_t *ptr;

            ptr = (uint64_t *)&dataSpaceBuf[0];
            data = *ptr;

            HAL_FLASH_Program(TYPEPROGRAM_DOUBLEWORD, address, data);
        }
    }

    HAL_FLASH_Lock();
    __enable_irq();
}
