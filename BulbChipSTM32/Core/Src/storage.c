#include "storage.h"

uint32_t getHexAddressOfPage(uint32_t dataPage){
	uint32_t bits       = PAGE_SECTOR * dataPage;
	uint32_t hexAddress = FLASH_INIT + bits;
	return hexAddress;
}

void memoryPageErase(uint32_t memoryPage){
	HAL_StatusTypeDef eraseHandler = HAL_FLASH_Unlock();

	eraseHandler = FLASH_WaitForLastOperation(500);
	__HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR);
	FLASH_PageErase(memoryPage);

	eraseHandler = FLASH_WaitForLastOperation(500);
	CLEAR_BIT(FLASH->CR, FLASH_CR_PER);
	HAL_FLASH_Lock();
}

void writeBytes(uint32_t page, uint8_t buf[], uint32_t bufCount)
{
	uint8_t numBytesToWrite = 8;
	memoryPageErase(page);

	HAL_FLASH_Unlock();

	uint8_t emptyPaddingLength = numBytesToWrite - (bufCount % numBytesToWrite);
	uint8_t dataSpaceBuf[numBytesToWrite];

	// write 8 bytes at a time, pad with \0 at end if bytes count is not a multiple of 8
	for (int32_t i = 0; i < bufCount + emptyPaddingLength; i++)
	{
		if (i < bufCount) {
			dataSpaceBuf[i % numBytesToWrite] = buf[i];
		} else {
			dataSpaceBuf[i % numBytesToWrite] = '\0';
		}

		if (i % 8 == 8 - 1) {
			uint32_t address = getHexAddressOfPage(page) + ((numBytesToWrite * (i / numBytesToWrite)));

			uint64_t data, *ptr;

			ptr = (uint64_t*)&dataSpaceBuf[0];
			data = *ptr;

			HAL_FLASH_Program(TYPEPROGRAM_DOUBLEWORD, address, data);
		}
	}

	HAL_FLASH_Lock();
}

void readTextFromFlash(uint32_t page, char* buffer, uint32_t length) {
	uint32_t address = getHexAddressOfPage(page);
    memcpy(buffer, (void*)address, length);
    buffer[length] = '\0'; // Null-terminate the string
}
