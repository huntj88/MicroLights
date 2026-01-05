#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "unity.h"

// Include the mock header
#include "stm32c0xx.h"

// Define the FLASH global
FLASH_TypeDef mockFlashPeripheral;
FLASH_TypeDef *FLASH = &mockFlashPeripheral;

// Mock implementations
HAL_StatusTypeDef HAL_FLASH_Unlock(void) {
    return HAL_OK;
}

HAL_StatusTypeDef FLASH_WaitForLastOperation(uint32_t Timeout) {
    return HAL_OK;
}

void FLASH_PageErase(uint32_t Page) {
    // In real hardware, this erases the page (sets to 0xFF)
    // We need to calculate the address from the page number.
    // We need to know the address of that page to erase it in our mmap-ed memory.
    // storage.h: #define FLASH_INIT 0x08000000
    // #define PAGE_SECTOR 2048

    uint32_t addr = 0x08000000 + (Page * 2048);
    memset((void *)(uintptr_t)addr, 0xFF, 2048);
}

HAL_StatusTypeDef HAL_FLASH_Lock(void) {
    return HAL_OK;
}

HAL_StatusTypeDef HAL_FLASH_Program(uint32_t TypeProgram, uint32_t Address, uint64_t Data) {
    // Write the data to the address
    *(uint64_t *)(uintptr_t)Address = Data;
    return HAL_OK;
}

void __HAL_FLASH_CLEAR_FLAG(uint32_t flags) {
    // Do nothing
}

// Include the source file under test
#include "../Core/Src/storage.c"

void setUp(void) {
    // Allocate memory at 0x08000000 to simulate Flash
    // We need enough for at least up to BULB_PAGE_0 + some modes.
    // BULB_PAGE_0 is 57. Let's allocate 1MB (512 pages).
    // 0x08000000 is the start.

    void *addr = (void *)(uintptr_t)0x08000000;
    size_t len = 1024 * 1024;  // 1MB

    // Use MAP_FIXED_NOREPLACE to force the address without overwriting.
    // Use MAP_ANONYMOUS | MAP_PRIVATE for memory.

    void *ptr = mmap(
        addr,
        len,
        PROT_READ | PROT_WRITE,
        MAP_FIXED_NOREPLACE | MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0);

    if (ptr == MAP_FAILED) {
        printf("mmap failed: %s\n", strerror(errno));
        TEST_FAIL_MESSAGE("Failed to mmap flash memory simulation");
    }

    // Initialize to 0xFF (erased state)
    memset(ptr, 0xFF, len);
}

void tearDown(void) {
    // munmap
    munmap((void *)(uintptr_t)0x08000000, 1024 * 1024);
}

void test_writeSettingsToFlash_WritesDataCorrectly(void) {
    const char *testData = "TestSettings";
    writeSettingsToFlash(testData, strlen(testData));

    char readBuf[32];
    readSettingsFromFlash(readBuf, sizeof(readBuf));

    TEST_ASSERT_EQUAL_STRING(testData, readBuf);
}

void test_writeSettingsToFlash_TruncatesIfTooLong(void) {
    // Create a buffer larger than PAGE_SECTOR
    char largeBuf[PAGE_SECTOR + 100];
    memset(largeBuf, 'A', sizeof(largeBuf));
    largeBuf[sizeof(largeBuf) - 1] = '\0';

    writeSettingsToFlash(largeBuf, strlen(largeBuf));

    char readBuf[PAGE_SECTOR + 100];
    readSettingsFromFlash(readBuf, PAGE_SECTOR);  // Read up to page sector

    // The stored string should be truncated to PAGE_SECTOR - 1
    // And null terminated.

    size_t len = strlen(readBuf);
    TEST_ASSERT_EQUAL_UINT32(PAGE_SECTOR - 1, len);
    TEST_ASSERT_EQUAL_CHAR('A', readBuf[0]);
    TEST_ASSERT_EQUAL_CHAR('A', readBuf[PAGE_SECTOR - 2]);
    TEST_ASSERT_EQUAL_CHAR('\0', readBuf[PAGE_SECTOR - 1]);
}

void test_writeBulbModeToFlash_WritesDataCorrectly(void) {
    const char *testData = "ModeData";
    uint8_t mode = 1;
    writeBulbModeToFlash(mode, testData, strlen(testData));

    char readBuf[32];
    readBulbModeFromFlash(mode, readBuf, sizeof(readBuf));

    TEST_ASSERT_EQUAL_STRING(testData, readBuf);
}

void test_readSettingsFromFlash_ReturnsEmptyString_WhenFlashIsErased(void) {
    // Flash is initialized to 0xFF in setUp
    // But writeSettingsToFlash hasn't been called.
    // readSettingsFromFlash should handle 0xFF.

    char readBuf[32];
    readSettingsFromFlash(readBuf, sizeof(readBuf));

    TEST_ASSERT_EQUAL_STRING("", readBuf);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_readSettingsFromFlash_ReturnsEmptyString_WhenFlashIsErased);
    RUN_TEST(test_writeBulbModeToFlash_WritesDataCorrectly);
    RUN_TEST(test_writeSettingsToFlash_TruncatesIfTooLong);
    RUN_TEST(test_writeSettingsToFlash_WritesDataCorrectly);
    return UNITY_END();
}
